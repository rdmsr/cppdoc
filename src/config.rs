use anyhow::Result;
use serde::{Deserialize, Serialize};

#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct Config {
    pub project: Project,
    pub input: Input,
    pub pages: Pages,
    pub output: Output,
    pub doctests: Option<Doctest>,
}

#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct Project {
    pub name: String,
    pub version: String,
}

#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct Input {
    pub glob: String,
    pub compiler_arguments: Vec<String>,
}

#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct Pages {
    pub index: Option<String>,
    pub extra: Option<Vec<String>>,
}

#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct Output {
    pub static_dir: String,
    pub path: String,
    pub root_namespace: Option<String>,
    pub base_url: String,
}

#[derive(Deserialize, Serialize, Clone, Debug)]
pub struct Doctest {
    pub enable: bool,
    pub run: Option<bool>,
    pub compiler_invocation: Option<Vec<String>>,
}

impl Config {
    pub fn new(config_file: &str) -> Result<Self> {
        let config_str = std::fs::read_to_string(config_file)?;

        let config: Config = toml::from_str(&config_str)?;
        Ok(config)
    }
}
