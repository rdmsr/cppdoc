use crate::config;
use crate::report::report_error;
use std::io::Write;
use tempfile::NamedTempFile;

pub struct Doctest {
    pub code: String,
    pub add_main: bool,
    pub include: Vec<String>,
    pub display_code: String,
}

const PREPENDED_CODE: &str = r#"
#include <iostream>
#include <stdlib.h>

#ifdef ASSERT
#undef ASSERT
#endif

#define ASSERT_EQ(A, B) if (A != B) { std::cerr << "Assertion failed: '" << #A << "' != '"<< #B << "'" << std::endl; exit(1); }
#define ASSERT_NE(A, B) if (A == B) { std::cerr << "Assertion failed: '" << #A << "' == '"<< #B << "'" << std::endl; exit(1); }
#define ASSERT_FALSE(A) if (A) { std::cerr << "Assertion failed: '" << #A << "' is true" << std::endl; exit(1); }
#define ASSERT_GT(A, B) if (A <= B) { std::cerr << "Assertion failed: '" << #A << "' <= '"<< #B << "'" << std::endl; exit(1); }
#define ASSERT_LT(A, B) if (A >= B) { std::cerr << "Assertion failed: '" << #A << "' >= '"<< #B << "'" << std::endl; exit(1); }
#define ASSERT_GE(A, B) if (A < B) { std::cerr << "Assertion failed: '" << #A << "' < '"<< #B << "'" << std::endl; exit(1); }
#define ASSERT_LE(A, B) if (A > B) { std::cerr << "Assertion failed: '" << #A << "' > '"<< #B << "'" << std::endl; exit(1); }

#define ASSERT(A) if (!(A)) { std::cerr << "Assertion failed: '" << #A << "' is false" << std::endl; exit(1); }
"#;

impl Doctest {
    pub fn new(code: String, add_main: bool) -> Self {
        let mut full_code = String::new();
        let mut display_code = String::new();
        let mut include: Vec<String> = Vec::new();

        for line in code.lines() {
            if line.trim().starts_with("@") {
                if line.trim().starts_with("@include") {
                    include.push(line.trim_start_matches("@include").trim().to_string());
                } else {
                    full_code.push_str(line.trim_start_matches("@"));
                    full_code.push_str("\n");
                }
            } else {
                display_code.push_str(line);
                display_code.push_str("\n");

                full_code.push_str(line);
                full_code.push_str("\n");
            }
        }

        Doctest {
            code: full_code,
            add_main,
            include,
            display_code,
        }
    }

    pub fn compile(&self, config: &config::Doctest) -> std::path::PathBuf {
        let compiler_invocation = config.compiler_invocation.join(" ");

        let mut in_file = tempfile::Builder::new().suffix(".cpp").tempfile().unwrap();

        let out_file = NamedTempFile::new().unwrap();

        let compiler_invocation =
            compiler_invocation.replace("{file}", in_file.path().to_str().unwrap());

        let compiler_invocation =
            compiler_invocation.replace("{out}", out_file.path().to_str().unwrap());

        let mut command = std::process::Command::new("sh");
        command.arg("-c").arg(&compiler_invocation);

        let orig_code = self.code.to_string();

        if self.add_main {
            let includes = self
                .include
                .iter()
                .map(|i| format!("#include {}", i))
                .collect::<Vec<String>>()
                .join("\n");

            let code = format!(
                "{}{}\nint main() {{\n{}\n return 0;}}",
                includes, PREPENDED_CODE, orig_code
            );
            in_file.write_all(code.as_bytes()).unwrap();
        } else {
            let includes = self
                .include
                .iter()
                .map(|i| format!("#include {}", i))
                .collect::<Vec<String>>()
                .join("\n");

            let code = format!("{}{}{}", includes, PREPENDED_CODE, orig_code);
            in_file.write_all(code.as_bytes()).unwrap();
        }

        let output = command.output().unwrap();

        in_file.close().unwrap();

        if !output.status.success() {
            report_error(&format!(
                "Could not compile test:\n{}\n\x1b[1mFull code:\x1b[0m\n{}",
                String::from_utf8_lossy(&output.stderr),
                orig_code
            ));
            std::process::exit(1);
        }

        out_file.keep().unwrap().1
    }

    pub fn run(&self, file_path: std::path::PathBuf) {
        let output = std::process::Command::new(file_path.clone())
            .output()
            .unwrap();

        if !output.status.success() {
            report_error(&format!(
                "Test failed:\n{}\n\x1b[1mFull code:\x1b[0m\n{}",
                String::from_utf8_lossy(&output.stderr),
                self.code,
            ));
        }

        std::fs::remove_file(file_path).unwrap();
    }
}
