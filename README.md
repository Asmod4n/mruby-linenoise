# mruby-linenoise
mruby wrapper for https://github.com/antirez/linenoise

```ruby
Linenoise.completion do |buf|
  if buf[0] == 'h'
    ['hello', 'hello there']
  end
end

Linenoise.hints do |buf|
  if buf == "hello"
    Linenoise::Hint.new(" World", 35, true) # this is a Struct with the folowing fields: message, color, bold
    # you can also just return a String
  end
end

begin
Linenoise::History.load('history.txt')
rescue
end

while (line = linenoise("hallo> "))
  unless line.empty?
    puts "echo: #{line}"
    Linenoise::History.add(line)
    Linenoise::History.save("history.txt")
  end
end
```

Other functions
===============

```ruby
Linenoise.clear_screen # clears the screen
Linenoise.multi_line= # enables or disables multi line mode

Linenoise.print_key_codes # a special mode which shows key codes

Linenoise::History.max_len= # changes the max lines the history keeps, defaults to 100
```

Acknowledgements
================

This is using code from https://github.com/antirez/linenoise

Copyright (c) 2010-2014, Salvatore Sanfilippo <antirez at gmail dot com>
Copyright (c) 2010-2013, Pieter Noordhuis <pcnoordhuis at gmail dot com>

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
