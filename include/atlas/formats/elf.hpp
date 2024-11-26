#pragma once
#include <atlas/cstr.hpp>
#include <atlas/enum.hpp>
#include <atlas/io/cursor.hpp>
#include <atlas/io/endian.hpp>
#include <atlas/io/traits.hpp>
#include <atlas/option.hpp>
#include <atlas/slice.hpp>
#include <atlas/string_view.hpp>
#include <cstddef>
#include <cstdint>

// FIXME: Not sure how well this handles 32-bit
namespace Atlas::Formats::Elf {

// -- Definitions --
constexpr static size_t EI_MAG0 = 0;
constexpr static size_t EI_MAG1 = 1;
constexpr static size_t EI_MAG2 = 2;
constexpr static size_t EI_MAG3 = 3;

constexpr static uint8_t ELF_MAG[] = {0x7f, 'E', 'L', 'F'};

constexpr static size_t EI_CLASS = 4;

enum class Class : uint8_t {
  None = 0,
  Elf32 = 1,
  Elf64 = 2,
  First = None,
  Last = Elf64,
};

constexpr static size_t EI_DATA = 5;

enum class Data : uint8_t {
  None = 0,
  Lsb = 1,
  Msb = 2,
  First = None,
  Last = Msb,
};

constexpr static size_t EI_VERSION = 6;

enum class Version : uint8_t {
  None = 0,
  Current = 1,
  First = None,
  Last = Current,
};

constexpr static size_t EI_OSABI = 7;

enum class OsAbi : uint8_t {
  None = 0,
  Sysv = 0,
  HPUX = 1,
  NetBSD = 2,
  Linux = 3,
  Solaris = 6,
  Aix = 7,
  Irix = 8,
  FreeBSD = 9,
  Tru64 = 10,
  Modesto = 11,
  OpenBSD = 12,
  OpenVMS = 13,
  NSK = 14,
  Aros = 15,
  Standalone = 255,
  First = None,
  Last = Standalone,
};

constexpr static size_t EI_ABIVERSION = 8;
constexpr static size_t EI_PAD = 9;

constexpr static size_t EI_NIDENT = 16;

// I sure wish I had algebraic data types here
enum class ParsingError {
  InvalidMagic,
  InvalidValue,
  IoError,
};

enum class Machine : uint16_t {
  None = 0,
  Sparc = 2,
  X86 = 3,
  Mips = 8,
  PowerPC = 20,
  Arm = 40,
  SuperH = 42,
  Ia64 = 50,
  Amd64 = 62,
  AArch64 = 183,
  RiscV = 243,
  First = None,
  Last = RiscV,
};

enum class Type : uint16_t {
  None = 0,
  Relocatable = 1,
  Executable = 2,
  Shared = 3,
  Core = 4,
  First = None,
  Last = Core,
};

struct [[gnu::packed]] NativeHeader {
  uint8_t ident[EI_NIDENT];
  uint16_t type;
  uint16_t machine;
  uint32_t version;
  uint64_t entry;
  uint64_t phoff;
  uint64_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
};

struct Ident {
  Class ei_class;
  Data ei_data;
  Version ei_version;
  OsAbi ei_osabi;
};

struct Header {
  Ident ident;
  Type type;
  Machine machine;
  uint32_t version;
  uint64_t entry;
  uint64_t phoff;
  uint64_t shoff;
  uint32_t flags;
  uint16_t ehsize;
  uint16_t phentsize;
  uint16_t phnum;
  uint16_t shentsize;
  uint16_t shnum;
  uint16_t shstrndx;
};

constexpr static uint32_t PF_X = 1;
constexpr static uint32_t PF_W = 2;
constexpr static uint32_t PF_R = 4;

struct [[gnu::packed]] NativeProgramHeader {
  uint32_t type;
  uint32_t flags;
  uint64_t offset;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t filesz;
  uint64_t memsz;
  uint64_t align;
};

struct [[gnu::packed]] NativeSectionHeader {
  uint32_t name;
  uint32_t type;
  uint64_t flags;
  uint64_t addr;
  uint64_t offset;
  uint64_t size;
  uint32_t link;
  uint32_t info;
  uint64_t addralign;
  uint64_t entsize;
};

struct [[gnu::packed]] NativeSymbol {
  uint32_t name;
  uint8_t info;
  uint8_t other;
  uint16_t shndx;
  uint64_t value;
  uint64_t size;
};

// -- Parsing --
struct ProgramHeader {
  enum class Type {
    None = 0,
    Load = 1,
    Dynamic = 2,
    Interp = 3,
    Note = 4,
    Shlib = 5,
    Phdr = 6,
    Tls = 7,
    Loos = 0x60000000,
    Hios = 0x6fffffff,
    Loproc = 0x70000000,
    Hiproc = 0x7fffffff,
    First = None,
    Last = Hiproc,
  };

  Type type;

  struct {
    bool execute;
    bool write;
    bool read;
  } flags;

  uint64_t offset;
  uint64_t vaddr;
  uint64_t paddr;
  uint64_t filesz;
  uint64_t memsz;
  uint64_t align;

  template <Io::Endianness E>
  static constexpr Result<ProgramHeader, ParsingError>
  deserialize(Slice<uint8_t> data) {
    NativeProgramHeader phdr{};

    Array<uint8_t, sizeof(phdr)> buffer{};

    Io::Cursor cursor(data);

    if (cursor.read(buffer.as_slice()).unwrap() != sizeof(phdr)) {
      return Err(ParsingError::IoError);
    }

    memcpy(&phdr, buffer.data(), sizeof(phdr));

    phdr.paddr = Io::to_endian<uint64_t, E>(phdr.paddr);
    phdr.vaddr = Io::to_endian<uint64_t, E>(phdr.vaddr);
    phdr.filesz = Io::to_endian<uint64_t, E>(phdr.filesz);
    phdr.memsz = Io::to_endian<uint64_t, E>(phdr.memsz);
    phdr.offset = Io::to_endian<uint64_t, E>(phdr.offset);
    phdr.align = Io::to_endian<uint64_t, E>(phdr.align);
    phdr.flags = Io::to_endian<uint32_t, E>(phdr.flags);
    phdr.type = Io::to_endian<uint32_t, E>(phdr.type);

    ProgramHeader ret{};

    ret.type = TRYE(enum_cast<Type>(phdr.type), ParsingError::InvalidValue);
    ret.flags = {.execute = static_cast<bool>(phdr.flags & PF_X),
                 .write = static_cast<bool>(phdr.flags & PF_W),
                 .read = static_cast<bool>(phdr.flags & PF_R)};

    ret.offset = phdr.offset;
    ret.vaddr = phdr.vaddr;
    ret.paddr = phdr.paddr;
    ret.filesz = phdr.filesz;
    ret.memsz = phdr.memsz;
    ret.align = phdr.align;

    return Ok(ret);
  }
};

struct SectionHeader {
  uint32_t name;

  enum class Type : uint32_t {
    Null = 0,
    Progbits = 1,
    Symtab = 2,
    Strtab = 3,
    Rela = 4,
    Hash = 5,
    Dynamic = 6,
    Note = 7,
    Nobits = 8,
    Rel = 9,
    Shlib = 10,
    Dynsym = 11,
    Num = 12,
    LoProc = 0x70000000,
    HiProc = 0x7fffffff,
    LoUser = 0x80000000,
    HiUser = 0xffffffff,
    First = Null,
    Last = HiUser,
  };

  Type type;

  struct {
    bool write;
    bool alloc;
    bool exec_instr;
  } flags;

  template <Io::Endianness E>
  static Result<SectionHeader, ParsingError> deserialize(Slice<uint8_t> data) {
    NativeSectionHeader shdr{};

    Array<uint8_t, sizeof(shdr)> buffer{};

    Io::Cursor cursor(data);

    if (cursor.read(buffer.as_slice()).unwrap() != sizeof(shdr)) {
      return Err(ParsingError::IoError);
    }

    memcpy(&shdr, buffer.data(), sizeof(shdr));

    shdr.addr = Io::to_endian<uint64_t, E>(shdr.addr);
    shdr.offset = Io::to_endian<uint64_t, E>(shdr.offset);
    shdr.size = Io::to_endian<uint64_t, E>(shdr.size);
    shdr.link = Io::to_endian<uint32_t, E>(shdr.link);
    shdr.info = Io::to_endian<uint32_t, E>(shdr.info);
    shdr.addralign = Io::to_endian<uint64_t, E>(shdr.addralign);
    shdr.entsize = Io::to_endian<uint64_t, E>(shdr.entsize);
    shdr.flags = Io::to_endian<uint64_t, E>(shdr.flags);
    shdr.type = Io::to_endian<uint32_t, E>(shdr.type);
    shdr.name = Io::to_endian<uint32_t, E>(shdr.name);

    SectionHeader ret{};

    ret.type = TRYE(enum_cast<Type>(shdr.type), ParsingError::InvalidValue);
    ret.flags = {.write = static_cast<bool>(shdr.flags & 0x1),
                 .alloc = static_cast<bool>(shdr.flags & 0x2),
                 .exec_instr = static_cast<bool>(shdr.flags & 0x4)};
    ret.addr = shdr.addr;
    ret.offset = shdr.offset;
    ret.size = shdr.size;
    ret.link = shdr.link;
    ret.info = shdr.info;
    ret.addralign = shdr.addralign;
    ret.entsize = shdr.entsize;
    ret.name = shdr.name;

    return Ok(ret);
  }

  uint64_t addr;
  uint64_t offset;
  uint64_t size;
  uint32_t link;
  uint32_t info;
  uint64_t addralign;
  uint64_t entsize;
};

struct Symbol {
  uint32_t name;
  uint8_t info;
  uint8_t other;
  uint16_t shndx;
  uint64_t value;
  uint64_t size;

  template <Io::Endianness E>
  static Result<Symbol, ParsingError> deserialize(Slice<uint8_t> data) {
    NativeSymbol sym{};

    Array<uint8_t, sizeof(sym)> buffer{};

    Io::Cursor cursor(data);

    if (cursor.read(buffer.as_slice()).unwrap() != sizeof(sym)) {
      return Err(ParsingError::IoError);
    }

    memcpy(&sym, buffer.data(), sizeof(sym));

    sym.name = Io::to_endian<uint32_t, E>(sym.name);
    sym.shndx = Io::to_endian<uint16_t, E>(sym.shndx);
    sym.value = Io::to_endian<uint64_t, E>(sym.value);
    sym.size = Io::to_endian<uint64_t, E>(sym.size);

    Symbol ret{};
    ret.name = sym.name;
    ret.info = sym.info;
    ret.other = sym.other;
    ret.shndx = sym.shndx;
    ret.value = sym.value;
    ret.size = sym.size;

    return Ok(ret);
  }
};

class File {

public:
  File(Slice<uint8_t> &data) : cursor_(data) {}

  static Result<File, ParsingError> deserialize(Slice<uint8_t> data) {
    Header header{};

    Array<uint8_t, EI_NIDENT> ident{};

    auto cursor = Io::Cursor(data);

    if (!cursor.read(ident.as_slice())) {
      return Err(ParsingError::IoError);
    }

    if (ident[EI_MAG0] != ELF_MAG[0] || ident[EI_MAG1] != ELF_MAG[1] ||
        ident[EI_MAG2] != ELF_MAG[2] || ident[EI_MAG3] != ELF_MAG[3]) {
      return Err(ParsingError::InvalidMagic);
    }

    header.ident.ei_data =
        TRYE(enum_cast<Data>(ident[EI_DATA]), ParsingError::InvalidValue);

    header.ident.ei_class =
        TRYE(enum_cast<Class>(ident[EI_CLASS]), ParsingError::InvalidValue);

    header.ident.ei_version =
        TRYE(enum_cast<Version>(ident[EI_VERSION]), ParsingError::InvalidValue);

    header.ident.ei_osabi =
        TRYE(enum_cast<OsAbi>(ident[EI_OSABI]), ParsingError::InvalidValue);

    bool is_lsb = header.ident.ei_data == Data::Lsb;
    using Io::Endianness::Big;
    using Io::Endianness::Little;

    // This should probably use NativeHeader instead
    if (is_lsb) {
      header.type =
          TRYE(enum_cast<Type>(cursor.read<uint16_t, Little>().unwrap()),
               ParsingError::InvalidValue);

      header.machine =
          TRYE(enum_cast<Machine>(cursor.read<uint16_t, Little>().unwrap()),
               ParsingError::InvalidValue);
      header.version = cursor.read<uint32_t, Little>().unwrap();
      header.entry = cursor.read<uint64_t, Little>().unwrap();
      header.phoff = cursor.read<uint64_t, Little>().unwrap();
      header.shoff = cursor.read<uint64_t, Little>().unwrap();
      header.flags = cursor.read<uint32_t, Little>().unwrap();
      header.ehsize = cursor.read<uint16_t, Little>().unwrap();
      header.phentsize = cursor.read<uint16_t, Little>().unwrap();
      header.phnum = cursor.read<uint16_t, Little>().unwrap();
      header.shentsize = cursor.read<uint16_t, Little>().unwrap();
      header.shnum = cursor.read<uint16_t, Little>().unwrap();
      header.shstrndx = cursor.read<uint16_t, Little>().unwrap();
    } else {
      header.type = TRYE(enum_cast<Type>(cursor.read<uint16_t, Big>().unwrap()),
                         ParsingError::InvalidValue);

      header.machine =
          TRYE(enum_cast<Machine>(cursor.read<uint16_t, Big>().unwrap()),
               ParsingError::InvalidValue);
      header.version = cursor.read<uint32_t, Big>().unwrap();
      header.entry = cursor.read<uint64_t, Big>().unwrap();
      header.phoff = cursor.read<uint64_t, Big>().unwrap();
      header.shoff = cursor.read<uint64_t, Big>().unwrap();
      header.flags = cursor.read<uint32_t, Big>().unwrap();
      header.ehsize = cursor.read<uint16_t, Big>().unwrap();
      header.phentsize = cursor.read<uint16_t, Big>().unwrap();
      header.phnum = cursor.read<uint16_t, Big>().unwrap();
      header.shentsize = cursor.read<uint16_t, Big>().unwrap();
      header.shnum = cursor.read<uint16_t, Big>().unwrap();
      header.shstrndx = cursor.read<uint16_t, Big>().unwrap();
    }

    File elf(data);
    elf.header_ = header;
    return Ok(elf);
  }

  /// Return an iterator over the program headers
  [[nodiscard]] auto phdrs() {
    auto phoff = header_.phoff;
    auto phentsize = header_.phentsize;
    auto phnum = header_.phnum;

    auto next = [this, phoff, phentsize,
                 phnum]() mutable -> Option<ProgramHeader> {
      if (phnum == 0) {
        return NONE;
      }

      ProgramHeader phdr{};

      auto slice = cursor_.slice(phoff, phentsize);

      if (header_.ident.ei_data == Data::Lsb) {
        phdr =
            ProgramHeader::deserialize<Io::Endianness::Little>(slice).unwrap();
      } else {
        phdr = ProgramHeader::deserialize<Io::Endianness::Big>(slice).unwrap();
      }

      phoff += phentsize;
      phnum--;

      return phdr;
    };

    return Iterator<decltype(next)>(next);
  }

  [[nodiscard]] auto sections() {
    auto shoff = header_.shoff;
    auto shentsize = header_.shentsize;
    auto shnum = header_.shnum;

    auto next = [this, shoff, shentsize,
                 shnum]() mutable -> Option<SectionHeader> {
      if (shnum == 0) {
        return NONE;
      }

      SectionHeader shdr{};

      auto slice = cursor_.slice(shoff, shentsize);

      if (header_.ident.ei_data == Data::Lsb) {
        shdr =
            SectionHeader::deserialize<Io::Endianness::Little>(slice).unwrap();
      } else {
        shdr = SectionHeader::deserialize<Io::Endianness::Big>(slice).unwrap();
      }

      shoff += shentsize;
      shnum--;

      return shdr;
    };

    return Iterator<decltype(next)>(next);
  }

  [[nodiscard]] auto symbols(SectionHeader symtab) {
    auto shoff = symtab.offset;
    auto shentsize = symtab.entsize;
    auto shnum = symtab.size / shentsize;

    auto next = [this, shoff, shentsize, shnum]() mutable -> Option<Symbol> {
      if (shnum == 0) {
        return NONE;
      }

      Symbol sym{};

      auto slice = cursor_.slice(shoff, shentsize);

      if (header_.ident.ei_data == Data::Lsb) {
        sym = Symbol::deserialize<Io::Endianness::Little>(slice).unwrap();
      } else {
        sym = Symbol::deserialize<Io::Endianness::Big>(slice).unwrap();
      }

      shoff += shentsize;
      shnum--;

      return sym;
    };

    return Iterator<decltype(next)>(next);
  }

  [[nodiscard]] const StringView get_string(uint32_t index,
                                            uint32_t section_index) {
    auto shoff = header_.shoff;
    auto shentsize = header_.shentsize;

    auto shdr = cursor_.slice(
        shoff + static_cast<uint64_t>(shentsize) * section_index, shentsize);

    SectionHeader shstrtab{};

    if (header_.ident.ei_data == Data::Lsb) {
      shstrtab =
          SectionHeader::deserialize<Io::Endianness::Little>(shdr).unwrap();
    } else {
      shstrtab = SectionHeader::deserialize<Io::Endianness::Big>(shdr).unwrap();
    }

    auto slice = cursor_.slice(shstrtab.offset + index, 1);

    const char *raw_str = reinterpret_cast<const char *>(slice.data());

    return StringView(raw_str);
  }

  [[nodiscard]] const Header &header() const { return header_; }

private:
  Io::Cursor<Slice<uint8_t>> cursor_;
  Header header_;
};
} // namespace Atlas::Formats::Elf