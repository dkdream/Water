# Copper Grammar for water
# Parser syntax

file = ( rule )+ end-of-file

rule = NAME @declare EQUAL choice @define
     | space
     | comment
     | end-of-line

##         call-on-desent                       call-on-assent
tree = LABEL ( assign )? OPEN regex CLOSE @tree ( apply )?
leaf = LABEL ( assign )?                  @leaf ( apply )?

regex  = choice   ( COMMA regex  @tuple  )?
choice = sequence ( BAR   choice @select )?

sequence = prefix suffix @sequence
         | suffix

prefix = AND condition @assert ( prefix @sequence )?
       | NOT condition @not    ( prefix @sequence )?

condition = IDENTIFIER
          | tree
          | leaf

suffix = primary    ( operator )?
       | IDENTIFIER assign
       | IDENTIFIER ( operator )?

primary = OPEN IDENTIFIER assign CLOSE
        | tree
        | leaf
        | OPEN regex CLOSE

operator = STAR     @zero_plus
         | PLUS     @one_plus
         | OPTIONAL @maybe
         | SOPEN NUMBER DASH NUMBER SCLOSE @range

assign = ASSIGN EVENT @assign
apply  =        EVENT @sequence

IDENTIFIER = NAME !EQUAL
NAME       =     < name-head name-tail > !( ':' ) - @identifier
LABEL      =     < name-head name-tail >    ':'   - @label
EVENT      = '@' < name-head name-tail >          - @event
NUMBER     = < [0-9]+ > @number -

ASSIGN   = '->' blank?
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

name-head  = [a-zA-Z]
name-tail  = ( [-_]? ( [a-zA-Z0-9] )+ )*

-           = (space | comment)*
blank       = ( ' ' | '\t' )+
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' ( !end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.




