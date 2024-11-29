use clap::{Parser, Subcommand};
use glob::glob;
use indicatif::{ProgressBar, ProgressStyle};
use render::get_path_for_name;
use serde::Serialize;
use std::{path::Path, time::Duration};

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
                    },
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
                Some(ref x) => std::fs::read_to_string(x).unwrap(),
                None => match root_namespace.comment {
                    Some(ref comment) => comment.description.clone(),
                    None => String::new(),
                },
            };

            let index_html =
                render::process_markdown(&index, &output.index, &mut doctests, &config);

            let mut extra_pages = Vec::new();

            for g in &config.pages.extra.clone().unwrap_or_default() {
                for file in glob(g).expect("Failed to read glob pattern") {
                    match file {
                        Ok(page_path) => {
                            match std::fs::read_to_string(&page_path) {
                                Ok(source) => {
                                    let mut page =
                                        render::process_markdown(&source, &output.index, &mut doctests, &config);
                                    if page.title.is_empty() {
                                        page.title = page_path.file_name().unwrap().to_string_lossy().into_owned();
                                    }
                                    page.path = page_path;
                                    extra_pages.push(page);
                                },
                                Err(e) => {
                                    report_warning(&format!("Error reading extra file “{page_path:?}”: {e}"));
                                }
                            };
                        },
                        Err(e) => {
                            report_warning(&format!("Error reading extra file “{g}”: {e}"));
                        }
                    };
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
                let path = Path::new(&config.output.path).join(page.path.parent().unwrap_or_else(|| &Path::new("")));
                std::fs::create_dir_all(path).map_err(|e| {
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

            for page in &pages.extra {
                context.insert("content", &page.content);
                context.insert("title", &page.title);

                std::fs::write(
                    format!("{}/{}.html", config.output.path, page.path.display()),
                    tera.render("docpage", &context).unwrap(),
                )
                .map_err(|e| {
                    report_error(&format!("Error writing extra page file: {}", e));
                    std::process::exit(1);
                })
                .unwrap();
            }

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
            templates::output_namespace(root_namespace, &pages, &config, &output.index, &tera)
                .unwrap();
            bar.finish_and_clear();

            // Copy everything in the static directory to the output directory
            for entry in std::fs::read_dir(&config.output.static_dir).unwrap() {
                let entry = entry.unwrap();
                let path = entry.path();
                let filename = path.file_name().unwrap();
                let dest = format!("{}/{}", config.output.path, filename.to_str().unwrap());
                std::fs::copy(&path, &dest).unwrap();
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
