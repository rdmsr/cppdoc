pub fn report_error(error: &str) {
    eprintln!("\x1b[1;31mError\x1b[0m: {}", error);
    std::process::exit(1);
}
