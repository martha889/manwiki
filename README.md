# manwiki

`manwiki` is a small C terminal program that displays Wikipedia article summaries in a man page-like layout.

> This project was created with AI assistance.

## Features

- Fetches article data from the Wikipedia REST API
- Renders output with man-style sections (`NAME`, `ARTICLE`)
- Wraps text based on your terminal width
- Offers optional extended article content
  - interactive prompt: `Show more from this article? [y/N]`
  - CLI flag: `--more` or `-m`
- Preserves paragraph breaks in extended content for easier reading

## Requirements

- C compiler (`cc`, `clang`, or `gcc`)
- `make`
- `curl`
- Internet connection

## Build

```bash
make
```

## Usage

```bash
./manwiki "Alan Turing"
```

```bash
./manwiki --more "Alan Turing"
```

## Example Output

### Example 1 (default summary + prompt)

```text
                  manwiki(1)  Terminal Wiki Manual  manwiki(1)
--------------------------------------------------------------------------------
NAME
    Alan Turing - English computer scientist (1912–1954)

ARTICLE
    Alan Mathison Turing was an English mathematician, computer scientist,
    logician, cryptanalyst, philosopher and theoretical biologist. He was highly
    influential in the development of theoretical computer science...
--------------------------------------------------------------------------------
                              manwiki  2026-05-07

Show more from this article? [y/N]:
```

### Example 2 (`--more` extended content)

```text
$ ./manwiki --more "Alan Turing"
                  manwiki(1)  Terminal Wiki Manual  manwiki(1)
--------------------------------------------------------------------------------
NAME
    Alan Turing - English computer scientist (1912–1954)

ARTICLE
    Alan Mathison Turing was an English mathematician, computer scientist...
--------------------------------------------------------------------------------
                              manwiki  2026-05-07

MORE
    Alan Mathison Turing (...) was an English mathematician, computer scientist,
    logician, cryptanalyst, philosopher and theoretical biologist...

    Born in London, Turing was raised in southern England...

    After the war, Turing worked at the National Physical Laboratory...
...
```

## Clean

```bash
make clean
```
