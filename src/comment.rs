use crate::parser::Comment;

fn remove_prefix_and_clean(string: &str, prefix: &str) -> String {
    if let Some(stripped) = string.strip_prefix(prefix) {
        stripped.trim().to_string()
    } else {
        string.trim().to_string()
    }
}

pub fn parse_comment(string: String) -> Comment {
    let mut ret = Comment {
        brief: "".to_string(),
        description: "".to_string(),
        impl_: None,
    };

    let mut lines = string.lines();

    let mut brief = String::new();
    while let Some(line) = lines.next() {
        let trimmed = line.trim();
        if trimmed.is_empty() {
            break;
        }

        if !brief.is_empty() {
            brief.push(' ');
        }

        if trimmed.starts_with("///<") {
            brief.push_str(&remove_prefix_and_clean(trimmed, "///<"));
        } else {
            brief.push_str(&remove_prefix_and_clean(trimmed, "///"));
        }
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

    ret
}
