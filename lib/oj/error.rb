
module Oj

  class Error < StandardError
  end # Error

  # An Exception that is raised as a result of a parse error while parsing a JSON document.
  class ParseError < Error
  end # ParseError

  # An Exception that is raised as a result of a path being too deep.
  class DepthError < Error
  end # DepthError

  # An Exception that is raised if a file fails to load.
  class LoadError < Error
  end # LoadError

  # An Exception that is raised if there is a conflict with mimicing JSON
  class MimicError < Error
  end # MimicError

end # Oj
