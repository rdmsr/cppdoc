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

    let mut lines = string.lines();
    if lines.clone().peekable().peek() == Some(&"/// #[doc(hidden)]") {
        return None;
    }

    let mut brief = String::new();
    while let Some(line) = lines.next() {
        let trimmed = line.trim();
        let cleaned = if trimmed.starts_with("//<") {
            remove_prefix_and_clean(trimmed, "//<")
        } else {
            remove_prefix_and_clean(trimmed, "///")
        };

        if cleaned.is_empty() {
            break;
        }

        if !brief.is_empty() {
            brief.push(' ');
        }

        brief.push_str(&cleaned);
    }

    ret.brief = brief;

    let description = lines.fold(String::new(), |mut acc, line| {
        let cleaned_up = remove_prefix_and_clean(line.trim(), "///");

        if cleaned_up.starts_with("\\impl{") {
            let impl_ = cleaned_up
                .trim_start_matches("\\impl{")
                .trim_end_matches('}')
                .split(',')
                .map(|s| s.trim().to_string())
                .collect();
            ret.impl_ = Some(impl_);
            return acc;
        }

        acc.push_str(&cleaned_up);
        acc.push('\n');
        acc
    });

    ret.description = description.trim().to_string();

    Some(ret)
}
