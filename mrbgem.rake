MRuby::Gem::Specification.new('mruby-linenoise') do |spec|
  spec.license = 'Simplified BSD License'
  spec.author  = 'Hendrik Beskow'
  spec.summary = 'linenoise for mruby, a line editing library akin to readline'
  spec.add_dependency 'mruby-struct'
  spec.add_dependency 'mruby-errno'
end
