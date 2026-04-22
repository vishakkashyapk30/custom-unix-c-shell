# Custom Unix Shell

A feature-rich Unix shell implementation written in C with support for job control, signal handling, and various built-in commands.

## Features

- Command parsing and execution
- Job control with background and foreground process management
- Signal handling (SIGCHLD, SIGTSTP, SIGINT)
- Built-in commands:
  - `cd` - Change directory
  - `exit` - Exit the shell
  - `jobs` - List background jobs
  - `fg` - Bring background job to foreground
  - `bg` - Resume suspended background job
  - `history` - Show command history
  - `ping` - Ping network addresses
- Input/output redirection support
- Pipe support for command chaining
- Process activity tracking

## Project Structure

```
custom-unix-c-shell/
├── include/          - Header files
│   ├── activities.h
│   ├── builtins.h
│   ├── fg_bg.h
│   ├── input.h
│   ├── jobs.h
│   ├── parser.h
│   ├── ping.h
│   ├── prompt.h
│   ├── shell.h
│   └── signal_handlers.h
├── src/              - Source files
│   ├── activities.c
│   ├── builtins.c
│   ├── command.c
│   ├── fg_bg.c
│   ├── input.c
│   ├── jobs.c
│   ├── main.c
│   ├── parser.c
│   ├── ping.c
│   ├── prompt.c
│   └── signal_handlers.c
├── Makefile          - Build configuration
└── requirements.txt  - Python testing dependencies
```

## Building

To compile the shell, run:

```bash
make
```

This will generate the `shell` executable in the current directory.

## Running the Shell

Start the shell with:

```bash
./shell
```

The shell will display a prompt and accept commands interactively.

## Examples

Basic command execution:
```
> ls -la
> echo "Hello, World!"
```

Background job execution:
```
> sleep 100 &
> jobs
```

Foreground and background job control:
```
> sleep 100 &
[1] 12345
> bg
> fg %1
```

Command redirection:
```
> echo "test" > output.txt
> cat < input.txt > output.txt
```

## Testing

Run the test suite with Python:

```bash
python3 test.py
```

## Development

The shell supports:
- Interactive command input
- Command history tracking
- Job management
- Signal handling for process control
- Custom error messages

## Build Commands

- `make` - Build the shell
- `make clean` - Remove compiled files
- `make test` - Run test suite
