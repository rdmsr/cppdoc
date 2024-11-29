use crate::config::Config;
use crate::doctest;
use crate::parser;
use crate::report::report_warning;

use serde::Serialize;
use std::collections::HashMap;
use std::path::PathBuf;

use pulldown_cmark::{CodeBlockKind, Event, Tag, TagEnd};
use pygmentize::HtmlFormatter;

#[derive(Debug, Serialize)]
pub struct Page {
    pub title: String,
    pub content: String,
    pub path: PathBuf,
}

pub fn get_path_for_name(name: &str, index: &HashMap<String, String>) -> Option<String> {
    let kind = index.get(name)?;

    if kind == "namespace" {
        return Some(name.replace("::", "/"));
    }

    let name = name.trim_start_matches("::");

    if name.contains("::") {
        let parts = name.split("::");
        let basename = parts.clone().last()?.replace("/", "slash");

        let path = parts
            .clone()
            .take(parts.count() - 1)
            .collect::<Vec<&str>>()
            .join("/");

        return Some(format!("{}/{}.{}", path, kind, basename));
    }

    Some(format!("{}.{}", kind, name.replace("/", "slash")))
}

pub fn get_namespace_path(name: &str) -> String {
    name.replace("::", "/")
}

pub fn process_markdown(
    input: &str,
    index: &HashMap<String, String>,
    doctests: &mut Vec<doctest::Doctest>,
    config: &Config,
) -> Page {
    let mut code = String::new();
    let mut in_code_block = false;
    let mut code_lang = String::new();
    let mut in_metadata = false;
    let mut metadata = String::new();
    let mut title = String::new();

    let parser = pulldown_cmark::Parser::new_ext(
        input,
        pulldown_cmark::Options::ENABLE_YAML_STYLE_METADATA_BLOCKS,
    )
    .filter_map(|event| match event {
        // -- Add support for mermaid code blocks and syntax highlighting --
        Event::Start(Tag::CodeBlock(CodeBlockKind::Fenced(lang))) => {
            code_lang = lang.to_string();
            in_code_block = true;
            None
        }
        Event::End(TagEnd::CodeBlock) => {
            if code_lang == "mermaid" {
                Some(Event::Html(
                    format!("<div class=\"mermaid\">{}</div>", code).into(),
                ))
            } else {
                if code_lang == "cpp" || code_lang == "c++" || code_lang.is_empty() {
                    let doctest = doctest::Doctest::new(code.clone(), true);

                    code = doctest.display_code.to_string();

                    doctests.push(doctest);

                    code_lang = "cpp".to_string();
                }

                if code_lang == "nomain" || code_lang == "c++-nomain" || code_lang == "cpp-nomain" {
                    let doctest = doctest::Doctest::new(code.clone(), false);

                    code = doctest.display_code.to_string();

                    doctests.push(doctest);

                    code_lang = "cpp".to_string();
                }

                // Pygmentize was chosen over syntect because it has way more themes and is customizable through a CSS stylesheet
                let ret = match pygmentize::highlight(&code, Some(&code_lang), &HtmlFormatter::new()) {
                    Ok(html) => Some(Event::Html(html.into())),
                    Err(_) => {
                        report_warning(&format!("Unable to create syntax highlighting for “{code_lang}” code block"));
                        Some(Event::Code(code.clone().into()))
                    },
                };
                code.clear();
                ret
            }
        }

        Event::Text(text) => {
            if in_code_block {
                code.push_str(&text);
                None
            } else if in_metadata {
                metadata.push_str(&text);
                None
            } else {
                Some(Event::Text(text))
            }
        }

        // -- Metadata --
        Event::Start(Tag::MetadataBlock(_)) => {
            in_metadata = true;
            None
        }

        Event::End(TagEnd::MetadataBlock(_)) => {
            in_metadata = false;
            title = metadata
                .lines()
                .find(|line| line.starts_with("title:"))
                .map(|line| line.trim_start_matches("title:"))
                .unwrap_or_default()
                .trim()
                .to_string();
            metadata.clear();
            None
        }

        // -- Add support for documentation links, these start with a :: in the URL --
        Event::Start(Tag::Link {
            link_type,
            dest_url,
            title,
            id,
        }) => {
            if let pulldown_cmark::CowStr::Borrowed(url) = dest_url {
                if url.starts_with("::") {
                    let url = url.trim_start_matches("::");
                    let real = get_path_for_name(url, index);

                    if let Some(real) = real {
                        return Some(Event::Html(
                            format!("<a href=\"{}/{}.html\">", config.output.base_url, real).into(),
                        ));
                    }
                }
            }

            Some(Event::Start(Tag::Link {
                link_type,
                dest_url,
                title,
                id,
            }))
        }

        Event::End(TagEnd::Link {}) => Some(Event::Html("</a>".into())),

        _ => Some(event),
    });

    let mut html_output = String::new();
    pulldown_cmark::html::push_html(&mut html_output, parser);
    Page {
        content: html_output,
        title,
        path: PathBuf::new()
    }
}

pub fn process_function(
    func: &mut parser::Function,
    index: &HashMap<String, String>,
    doctests: &mut Vec<doctest::Doctest>,
    config: &Config,
) {
    if let Some(ref mut comment) = &mut func.comment {
        comment.brief = process_markdown(&comment.brief, index, doctests, config).content;
        comment.description =
            process_markdown(&comment.description, index, doctests, config).content;
    }
}

pub fn process_enum(
    enm: &mut parser::Enum,
    index: &HashMap<String, String>,
    doctests: &mut Vec<doctest::Doctest>,
    config: &Config,
) {
    if let Some(ref mut comment) = &mut enm.comment {
        comment.brief = process_markdown(&comment.brief, index, doctests, config).content;
        comment.description =
            process_markdown(&comment.description, index, doctests, config).content;
    }
}

pub fn process_record(
    record: &mut parser::Record,
    index: &HashMap<String, String>,
    doctests: &mut Vec<doctest::Doctest>,
    config: &Config,
) {
    if let Some(ref mut comment) = &mut record.comment {
        comment.brief = process_markdown(&comment.brief, index, doctests, config).content;
        comment.description =
            process_markdown(&comment.description, index, doctests, config).content;
    }

    for method in &mut record.methods {
        process_function(method, index, doctests, config);
    }

    for ctor in &mut record.ctor {
        process_function(ctor, index, doctests, config);
    }
}

pub fn process_namespace(
    namespace: &mut parser::Namespace,
    index: &HashMap<String, String>,
    doctests: &mut Vec<doctest::Doctest>,
    config: &Config,
) {
    if let Some(ref mut comment) = &mut namespace.comment {
        comment.brief = process_markdown(&comment.brief, index, doctests, config).content;
        comment.description =
            process_markdown(&comment.description, index, doctests, config).content;
    }

    for func in &mut namespace.functions {
        process_function(func, index, doctests, config);
    }

    for record in &mut namespace.records {
        process_record(record, index, doctests, config);
    }

    for enm in &mut namespace.enums {
        process_enum(enm, index, doctests, config);
    }

    for ns in &mut namespace.namespaces {
        process_namespace(ns, index, doctests, config);
    }
}
