# Copper Grammar for water
# Parser syntax

file = ( rule )+ end-of-file

rule = NAME @declare EQUAL ( tree | leaf ) @define
     | space
     | comment
     | end-of-line

tree = LABEL ( assign )? OPEN regex CLOSE @tree
leaf = LABEL ( assign )?                  @leaf

regex  = choice   ( COMMA regex  @tuple  )?
choice = sequence ( BAR   choice @select )?

sequence = prefix suffix @sequence ( THUNK @sequence )?
         | suffix                  ( THUNK @sequence )?

prefix = AND condition @assert ( prefix @sequence )?
       | NOT condition @not    ( prefix @sequence )?
       | THUNK

condition = IDENTIFIER
          | tree
          | leaf

suffix = primary    ( operator )?
       | IDENTIFIER assign
       | IDENTIFIER

primary = OPEN IDENTIFIER assign CLOSE
        | tree
        | leaf
        | OPEN regex CLOSE

operator = STAR     @zero_plus
         | PLUS     @one_plus
         | OPTIONAL @maybe
         | SOPEN NUMBER DASH NUMBER SCLOSE @range

assign = ASSIGN @assign

IDENTIFIER = NAME !EQUAL
NAME       = < name-head  name-tail > !( ':' )           - @identifier
LABEL      = < label-head name-tail >    ':'             - @label
ASSIGN     = '->' blank? < variable-head variable-tail > - @variable
NUMBER     = < [0-9]+ > @number -
THUNK      = '{' < braces* > '}' - @thunk

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
AND      = '&' -
NOT      = '!' -




name-head     = [A-Z]
label-head    = [a-zA-Z]
variable-head = [a-z]
name-tail     = ( [-_]? ( [a-zA-Z0-9] )+ )*
variable-tail = ( [-_]? ( [a-z0-9] )+ )*
braces        = '{' braces* '}'
              |  !'}' .

-           = (space | comment)*
blank       = ( ' ' | '\t' )+
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' ( !end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.




