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
    Linenoise::Hint.new(" World", 35, true)
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
