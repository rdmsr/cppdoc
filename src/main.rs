use clap::{Parser, Subcommand};
use glob::glob;
use indicatif::{ProgressBar, ProgressStyle};
use render::get_path_for_name;
use serde::Serialize;
use std::collections::HashMap;
use std::{path::Path, time::Duration};

mod book;
mod comment;
mod config;
mod doctest;
mod parser;
mod render;
mod report;
mod templates;

use report::{report_error, report_warning};

#[derive(Parser, Debug)]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Serialize)]
struct Pages {
    index: render::Page,
    extra: Vec<render::Page>,
}

#[derive(Serialize)]
struct SearchIndex {
    id: i32,
    name: String,
    link: String,
    kind: String,
}

#[derive(Subcommand, Debug)]
enum Commands {
    #[clap(name = "build", about = "Build documentation for the project")]
    Build {
        /// Dump JSON output
        #[clap(short, long)]
        dump_json: bool,

        /// Configuration file to use
        #[arg(short, long, default_value = "cppdoc.toml", value_name = "FILE")]
        config_file: Option<String>,
    },
}

fn render_page(
    page: &book::Page,
    index: &HashMap<String, String>,
    doctests: &mut Vec<doctest::Doctest>,
    config: &config::Config,
) -> Vec<render::Page> {
    let mut ret = vec![];

    match std::fs::read_to_string(&page.path) {
        Ok(source) => {
            let mut page_ret = render::process_markdown(&source, &index, doctests, &config);

            page_ret.title = page.name.clone();
            page_ret.path = Path::new(&page.path).to_path_buf();

            for sub_page in &page.sub_pages {
                ret.extend(render_page(&sub_page, &index, doctests, &config));
            }

            ret.push(page_ret);

            ret
        }
        Err(e) => {
            report_warning(&format!("Error reading page \"{0}\": {e}", page.path));
            vec![]
        }
    }
}

fn main() {
    let args = Cli::parse();

    match args.command {
        Commands::Build {
            dump_json,
            config_file,
        } => {
            let config_file = config_file.unwrap_or("cppdoc.toml".to_string());

            let config = match config::Config::new(&config_file) {
                Ok(config) => config,
                Err(e) => {
                    eprintln!("Error reading config file: {}", e);
                    std::process::exit(1);
                }
            };

            let clang = clang::Clang::new().unwrap();
            let mut parser = parser::Parser::new(&clang);

            let mut output: parser::Output = Default::default();

            let bar = ProgressBar::new_spinner();

            for file in glob(&config.input.glob).expect("Failed to read glob pattern") {
                match file {
                    Ok(file) => {
                        bar.set_message(format!("Parsing {}", file.to_str().unwrap()));
                        parser.parse(&config, file.to_str().unwrap(), &mut output);
                        bar.tick();
                    }
                    Err(e) => {
                        report_warning(&format!("Error reading input file: {e:}"));
                    }
                };
            }

            bar.finish_and_clear();

            if dump_json {
                let json = serde_json::to_string_pretty(&output).unwrap();
                println!("{}", json);
                return;
            }

            let root_namespace = if let Some(ref root_namespace) = config.output.root_namespace {
                // Find namespace
                output
                    .root
                    .namespaces
                    .iter_mut()
                    .find(|ns| ns.name == *root_namespace)
                    .unwrap()
            } else {
                &mut output.root
            };

            let mut doctests = Vec::new();

            render::process_namespace(root_namespace, &output.index, &mut doctests, &config);

            let index = match config.pages.index {
                Some(ref x) => match std::fs::read_to_string(x) {
                    Ok(s) => s,
                    Err(e) => {
                        report_error(&format!("Could not read index file: {}", e));
                        "".into()
                    }
                },
                None => match root_namespace.comment {
                    Some(ref comment) => comment.description.clone(),
                    None => String::new(),
                },
            };

            let mut index_html =
                render::process_markdown(&index, &output.index, &mut doctests, &config);

            index_html.path = "index.html".into();

            let mut extra_pages = Vec::new();

            if let Some(_) = config.pages.extra {
                report_warning("The `extra` option is deprecated, please use `book` instead");
            }

            let mut summary: book::Summary = Default::default();

            if let Some(ref book_dir) = config.pages.book {
                let path = std::path::Path::new(&book_dir).join("SUMMARY.md");

                let contents = std::fs::read_to_string(path).unwrap();

                summary = book::parse_summary(&contents, book_dir);
            }

            for segment in &summary.segments {
                if let book::Segment::Page(p) = segment {
                    extra_pages.extend(render_page(&p, &output.index, &mut doctests, &config));
                }
            }

            let pages = Pages {
                index: index_html,
                extra: extra_pages,
            };

            if let Some(ref doctest_conf) = config.doctests {
                if doctest_conf.enable {
                    let bar = ProgressBar::new(doctests.len() as u64);

                    bar.set_style(
                        ProgressStyle::with_template("Running doctest {pos}/{len}").unwrap(),
                    );

                    if let None = doctest_conf.run {
                        report_error("Doctests enabled but no run option specified");
                        std::process::exit(1);
                    }

                    if let None = doctest_conf.compiler_invocation {
                        report_error("Doctests enabled but no compiler invocation specified");
                        std::process::exit(1);
                    }

                    for doc in doctests {
                        let out = doc.compile(doctest_conf);

                        if doctest_conf.run.unwrap() {
                            doc.run(out);
                        }

                        bar.inc(1);
                    }

                    bar.finish_and_clear();
                }
            }

            // Make directories
            std::fs::create_dir_all(&config.output.path)
                .map_err(|e| {
                    report_error(&format!("Error creating output directory: {}", e));
                    std::process::exit(1);
                })
                .unwrap();

            for page in &pages.extra {
                let path = Path::new(&config.output.path)
                    .join(page.path.parent().unwrap_or_else(|| &Path::new("")));
                std::fs::create_dir_all(path)
                    .map_err(|e| {
                        report_error(&format!("Error creating output directory: {}", e));
                        std::process::exit(1);
                    })
                    .unwrap();
            }

            let tera = templates::init(&output.index, &config);
            let mut context = tera::Context::new();

            context.insert("config", &config);
            context.insert("project", &config.project);
            context.insert("pages", &pages);
            context.insert("summary", &summary);

            for page in &pages.extra {
                context.insert("content", &page.content);
                context.insert("title", &page.title);
                context.insert("page", &page);

                let mut out_path = Path::new(&config.output.path).join(&page.path);

                out_path.set_extension("md.html");

                std::fs::write(&out_path, tera.render("docpage", &context).unwrap())
                    .map_err(|e| {
                        report_error(&format!(
                            "Error writing extra page file {}: {}",
                            out_path.display(),
                            e
                        ));
                        std::process::exit(1);
                    })
                    .unwrap();
            }

            context.insert("page", &pages.index);

            std::fs::write(
                format!("{}/search.html", config.output.path),
                tera.render("search", &context).unwrap(),
            )
            .map_err(|e| {
                report_error(&format!("Error writing search page file: {}", e));
                std::process::exit(1);
            })
            .unwrap();

            let bar = ProgressBar::new_spinner();
            bar.enable_steady_tick(Duration::from_millis(100));
            bar.set_message("Rendering root namespace");

            if let Err(e) = templates::output_namespace(
                root_namespace,
                &pages,
                &config,
                &output.index,
                &summary,
                &tera,
            ) {
                report_error(&format!("Could not render root namespace: {}", e));
            }

            bar.finish_and_clear();

            match std::fs::read_dir(&config.output.static_dir) {
                Ok(dir) => {
                    // Copy everything in the static directory to the output directory
                    for entry in dir {
                        let entry = entry.unwrap();
                        let path = entry.path();
                        let filename = path.file_name().unwrap();
                        let dest = format!("{}/{}", config.output.path, filename.to_str().unwrap());
                        std::fs::copy(&path, &dest).unwrap();
                    }
                }

                Err(e) => {
                    report_error(&format!("Could not copy static directory: {}", e));
                }
            }

            // Make a new, more searchable index
            let mut id: i32 = 0;
            let mut index = Vec::new();

            for item in &output.index {
                index.push(SearchIndex {
                    id,
                    name: item.0.clone().replace("\"", "&quot;"),
                    link: match item.1.as_str() {
                        "namespace" => {
                            format!(
                                "{}/index",
                                get_path_for_name(item.0, &output.index).unwrap_or_default()
                            )
                        }
                        _ => get_path_for_name(item.0, &output.index).unwrap_or_default(),
                    }
                    .replace("\"", "&quot;")
                    .to_string(),

                    kind: item.1.clone(),
                });

                id += 1;
            }

            // Add pages to the search index
            for page in &pages.extra {
                index.push(SearchIndex {
                    id,
                    name: page.title.clone(),
                    link: page.path.to_string_lossy().into_owned(),
                    kind: "page".to_string(),
                });

                id += 1;
            }

            let index_json = serde_json::to_string_pretty(&index).unwrap();

            std::fs::write(
                format!("{}/search_index.json", config.output.path),
                index_json,
            )
            .unwrap();

            println!("Documentation generated in {}", config.output.path);
        }
    }
}
