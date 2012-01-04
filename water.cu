# Copper Grammar for water
# Parser syntax

file = ( rule )+ end-of-file

rule = NAME @declare EQUAL node_choice @define
     | space
     | comment
     | end-of-line

node_choice = node ( BAR node_choice @or )?

node = node_prefix node_suffix @and
     | node_suffix

node_prefix = AND test_node @assert ( node_prefix @and )?
            | NOT test_node @not    ( node_prefix @and )?
            | predicate             ( node_prefix @and )?

node_suffix = IDENTIFIER ( assign )?
            | node_value

test_node = IDENTIFIER
          | node_value

node_value = leaf | tree | root

leaf = label ( assign )? LEAF @leaf
     | label LEAF ( assign )? @leaf
tree = label ( assign )? SOPEN regex SCLOSE @tree ( apply )?
root = label ( assign )?

## call-on-desent event
assign = ASSIGN EVENT @assign

## call-on-assent event
apply  = EVENT @and

regex    = choice   ( COMMA regex  @tuple  )?
choice   = sequence ( BAR   choice @select )?
sequence = OPEN node_choice CLOSE operator
         | OPEN regex       CLOSE ( operator )?
         | node_prefix OPEN node_choice CLOSE @sequence operator
         | node_prefix OPEN regex       CLOSE @sequence ( operator )?
         | node_choice

operator = STAR     @zero_plus
         | PLUS     @one_plus
         | OPTIONAL @maybe
         | COPEN NUMBER DASH NUMBER CCLOSE @range

label     = LABEL | predicate
predicate = ANY   | !LEAF PREDICATE

IDENTIFIER = NAME !EQUAL @identifier
NAME       =     < name-head name-tail > !( ':' ) -
LABEL      =     < name-head name-tail >    ':'   - @label
EVENT      = '@' < name-head name-tail >          - @event
PREDICATE  = '%' < name-head name-tail >          - @predicate
NUMBER     = < [0-9]+ > @number -

ASSIGN   = '->' blank?
LEAF     = '%leaf' -
EQUAL    = '=' -
STAR     = '*' -
PLUS     = '+' -
OPTIONAL = '?' -
BAR      = '|' -
COMMA    = ',' -
OPEN     = '(' -
CLOSE    = ')' -
SOPEN    = '[' -
SCLOSE   = ']' -
COPEN    = '{' -
CCLOSE   = '}' -
AND      = '&' -
NOT      = '!' -
ANY      = '%any' - @any



name-head  = ( [a-zA-Z] )+
name-tail  = ( [-_]? ( [a-zA-Z0-9] )+ )*

-           = (space | comment)*
blank       = ( ' ' | '\t' )+
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' ( !end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.




