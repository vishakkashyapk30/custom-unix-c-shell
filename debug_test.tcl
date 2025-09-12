#!/usr/bin/expect -f

set shell_executable "./shell.out"
set timeout 3

set user [exec id -un]
set hostname [exec hostname]
set shell_prompt_regex "<${user}@${hostname}:\[^>]+> "

puts "Testing shell interaction..."

spawn -noecho $shell_executable
expect -re $shell_prompt_regex

puts "Sending 'hop -' command..."
send "hop -\r"
expect -re ".*~> " {
    puts "Got expected prompt after hop -"
} timeout {
    puts "Timeout waiting for prompt after hop -"
    exit 1
}

puts "Sending 'hop .test' command..."
send "hop .test\r"
expect -re ".*\\.test> " {
    puts "Got expected prompt after hop .test"
} timeout {
    puts "Timeout waiting for prompt after hop .test"
    exit 1
}

puts "Sending 'pwd' command..."
send "pwd\r"
expect -re ".*\\.test" {
    puts "Got expected pwd output"
} timeout {
    puts "Timeout waiting for pwd output"
    exit 1
}

puts "Sending 'hop -' command..."
send "hop -\r"
expect -re ".*~> " {
    puts "Got expected prompt after hop -"
} timeout {
    puts "Timeout waiting for prompt after hop -"
    exit 1
}

puts "Sending logout command..."
send "logout\r"
expect eof
catch {wait}

puts "Test completed successfully"
