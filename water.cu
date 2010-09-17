# Copper Grammar for water
# Parser syntax

file = ( rule )+ end-of-file

rule = NAME EQUAL ( tree | leaf ) @define
     | space
     | comment
     | end-of-line

tree = LABEL ( assign )? regex @tree
leaf = LABEL ( assign )?       @leaf

regex = OPEN @start choice ( COMMA choice )+ @end CLOSE @tuble
      | OPEN choice CLOSE

choice = @start primary ( BAR primary )+ @end @select
       | primary
        
primary = OPEN IDENTIFIER assign @reference CLOSE ( operator )?
        | IDENTIFIER assign @reference
        | IDENTIFIER @reference ( operator )?
        | tree  ( operator )?
        | leaf  ( operator )?
        | regex ( operator )?


operator = STAR     @zero_plus
         | PLUS     @one_plus
         | OPTIONAL @maybe
         | range    @min_max

assign = ASSIGN @assign # stack: name variable 

range = SOPEN NUMBER DASH NUMBER SCLOSE @range

IDENTIFIER = NAME !EQUAL
NAME       = < name-head  name-tail > !( ':' ) @identifier -
LABEL      = < label-head name-tail >    ':'   @label -
ASSIGN     = '->' blank? < variable-head variable-tail >  @variable -
NUMBER     = < [0-9]+ > @number -
EQUAL      = '=' -
STAR       = '*' -
PLUS       = '+' -
OPTIONAL   = '?' -
BAR        = '|' -
COMMA      = ',' -
OPEN       = '(' -
CLOSE      = ')' -
SOPEN      = '[' -
SCLOSE     = ']' -

name-head     = [A-Z]
label-head    = [a-zA-Z]
variable-head = [a-z]
name-tail     = ( [-_]? ( [a-zA-Z0-9] )+ )*
variable-tail = ( [-_]? ( [a-z0-9] )+ )*

-           = (space | comment)*
blank       = ( ' ' | '\t' )+
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' ( !end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.




