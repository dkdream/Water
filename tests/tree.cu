
file = _ @begin_childern ( tree )+  @begin_childern end-of-file

tree = root begin ( node )+ end @push_tree

node = tree | leaf

leaf = name
     | string
     | number

root       = < name-head name-tail >    ':'   _ @push_symbol
name       = < name-head name-tail > !( ':' ) _ @push_symbol
name-head  = ( [a-zA-Z] )+
name-tail  = ( [-_]? ( [a-zA-Z0-9] )+ )*

string = quotation < ( !quotation . )* > quotation _ @push_string
number = < [-+]? ( [0-9] )+ > _                      @push_number

begin = '[' _ @begin_childern
end   = ']' _ @end_childern

_           = (space | comment)*
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' ( !end-of-line .)*
quotation   = ["]
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.
