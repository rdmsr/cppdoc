#pragma once

namespace Atlas {

enum Error {
  Unknown,
  NotImplemented,
  NotFound,
  IsADirectory,
  NotADirectory,
  PermissionDenied,
  InvalidParameters,
  InvalidFile,
  OutOfMemory,
  Full,
  Empty,
  Duplicate,
  OutOfBounds,
};

static inline constexpr char const *error_string(Error error) {
  switch (error) {
  case Unknown:
    return "Unknown";
  case NotImplemented:
    return "Not implemented";
  case NotFound:
    return "Not found";
  case IsADirectory:
    return "Is a directory";
  case NotADirectory:
    return "Not a directory";
  case PermissionDenied:
    return "Permission denied";
  case InvalidParameters:
    return "Invalid parameters";
  case InvalidFile:
    return "Invalid file";
  case OutOfMemory:
    return "Out of memory";
  case Full:
    return "Full";
  case Empty:
    return "Empty";
  case Duplicate:
    return "Duplicate";
  case OutOfBounds:
    return "Out of bounds";
  }
  return "Unknown";
}

} // namespace Atlas
