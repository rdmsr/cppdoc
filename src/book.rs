use pulldown_cmark::{Event, Tag, TagEnd};
use serde::Serialize;

#[derive(Default, Clone, Debug, Serialize)]
pub struct Page {
    pub name: String,
    pub path: String,
    pub sub_pages: Vec<Page>,
    pub number: String,
    pub numbered: bool,
}

#[derive(Debug, Serialize)]
pub enum Segment {
    Section(String),
    Page(Page),
}

#[derive(Default, Serialize, Debug)]
pub struct Summary {
    pub segments: Vec<Segment>,
}

fn assign_numbers(pages: &mut [Page], prefix: Vec<i32>) {
    for (i, page) in pages.iter_mut().enumerate() {
        let mut current_prefix = prefix.clone();

        page.number = current_prefix
            .iter()
            .map(|n| n.to_string())
            .collect::<Vec<_>>()
            .join(".");

        current_prefix.push((i + 1) as i32);
        assign_numbers(&mut page.sub_pages, current_prefix);
    }
}

pub fn parse_summary(contents: &str, base_path: &str) -> Summary {
    let mut summary: Summary = Default::default();

    let mut link_url: String = String::new();
    let mut current_page: Page = Default::default();
    let mut text: String = String::new();

    let parser = pulldown_cmark::Parser::new(contents);

    let mut page_levels: Vec<Vec<Page>> = vec![vec![]];
    let mut list_depth = 0;

    for event in parser {
        match event {
            Event::Start(Tag::Link {
                link_type: _,
                dest_url,
                title: _,
                id: _,
            }) => {
                link_url = dest_url.to_string();
            }

            Event::End(TagEnd::Link) => {
                current_page.name = text.clone();

                current_page.path = std::path::Path::new(&base_path)
                    .join(&link_url)
                    .to_str()
                    .unwrap()
                    .to_string();

                current_page.numbered = list_depth != 0;

                if current_page.numbered {
                    if let Some(current_level) = page_levels.last_mut() {
                        current_level.push(current_page.clone());
                    }
                } else {
                    summary.segments.push(Segment::Page(current_page.clone()));
                }

                text.clear();
            }

            Event::Start(Tag::List(_)) => {
                list_depth += 1;
                page_levels.push(vec![]);
            }

            Event::End(TagEnd::List(_)) => {
                list_depth -= 1;

                if page_levels.len() > 1 {
                    let children = page_levels.pop().unwrap();
                    if let Some(parent_level) = page_levels.last_mut() {
                        if let Some(last_parent) = parent_level.last_mut() {
                            last_parent.sub_pages.extend(children);
                        } else {
                            for p in children {
                                summary.segments.push(Segment::Page(p));
                            }
                        }
                    }
                }
            }

            Event::Start(Tag::Item) => {
                current_page = Page::default();
            }

            Event::Text(t) => {
                text.push_str(&t);
            }

            Event::End(TagEnd::Heading(_)) => {
                summary.segments.push(Segment::Section(text.clone()));
                text.clear();
            }

            _ => {}
        }
    }

    let mut prefix = 1;
    for segment in &mut summary.segments {
        if let Segment::Page(ref mut page) = segment {
            if page.numbered {
                assign_numbers(std::slice::from_mut(page), vec![prefix]);
                prefix += 1;
            }
        }
    }

    summary
}
