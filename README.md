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
- Paginates extended content in interactive terminals using your pager (`$PAGER`, default: `less -R`)

## Requirements

- C compiler (`cc`, `clang`, or `gcc`)
- `make`
- `curl`
- Internet connection
- `less` (recommended for interactive pagination)

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
    (opens in pager: $PAGER or less -R)
    - Forward: Space / f
    - Backward: b
    - Search: /term
    - Quit: q

```

## Clean

```bash
make clean
```
