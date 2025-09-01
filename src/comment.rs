use crate::parser::Comment;

fn remove_prefix_and_clean(string: &str, prefix: &str) -> String {
    if let Some(stripped) = string.strip_prefix(prefix) {
        stripped.trim().to_string()
    } else {
        string.trim().to_string()
    }
}

pub fn parse_comment(string: String) -> Option<Comment> {
    let mut ret = Comment {
        brief: "".to_string(),
        description: "".to_string(),
        impl_: None,
    };

    let lines = string.lines();
    let trimmed = string.trim();

    // Skip hidden docs
    if trimmed == "/// #[doc(hidden)]" || trimmed == "/**< #[doc(hidden)]*/" {
        return None;
    }

    if trimmed.starts_with("/**<") {
        let content = trimmed
            .trim_start_matches("/**<")
            .trim_end_matches("*/")
            .trim();
        ret.brief = content.to_string();
        return Some(ret);
    }

    let is_block_comment = trimmed.starts_with("/**");
    let mut collected_lines = Vec::new();

    if is_block_comment {
        for line in lines.skip(1) {
            let trimmed = line.trim();

            if trimmed.ends_with("*/") {
                let without_end = trimmed
                    .trim_end_matches("*/")
                    .trim_start_matches('*')
                    .trim();
                if !without_end.is_empty() {
                    collected_lines.push(without_end.to_string());
                }
                break;
            }

            let cleaned = trimmed
                .trim_start_matches('*')
                .trim_start_matches('/')
                .trim();
            collected_lines.push(cleaned.to_string());
        }
    } else {
        collected_lines.extend(string.lines().map(|l| {
            if l.trim().starts_with("///<") {
                remove_prefix_and_clean(l.trim(), "///<")
            } else {
                remove_prefix_and_clean(l.trim(), "///")
            }
        }));
    }

    let mut iter = collected_lines.into_iter().filter(|l| !l.is_empty());
    if let Some(first_line) = iter.next() {
        ret.brief = first_line;
    }

    let mut description = String::new();
    for line in iter {
        if line.starts_with("\\impl{") {
            let impl_ = line
                .trim_start_matches("\\impl{")
                .trim_end_matches('}')
                .split(',')
                .map(|s| s.trim().to_string())
                .collect();
            ret.impl_ = Some(impl_);
            continue;
        }
        description.push_str(&line);
        description.push('\n');
    }

    ret.description = description.trim().to_string();

    Some(ret)
}
