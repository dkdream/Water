# Copper Grammar for water
# Parser syntax

rule = NAME EQUAL ( tree | leaf ) @define # stack: identifier node -> __
     | space
     | comment
     | end-of-line
     | end-of-file @done

tree = LABEL OPEN regex CLOSE @tree # stack: label node -> node
leaf = LABEL @leaf                  # stack: label -> node

regex   = @start choice  ( COMMA choice  )* @end @tuble  # stack: List<node> -> node
choice  = @start primary ( BAR   primary )* @end @select # stack: List<node> -> node
primary = prefix ( operator )?                           # see operator

prefix  = IDENTIFIER @reference # stack: identifier -> node
        | tree                  # see tree
        | OPEN regex CLOSE      # see regex

operator = STAR     @zero_plus  # stack: node -> node
         | PLUS     @one_plus   # stack: node -> node
         | OPTIONAL @maybe      # stack: node -> node
         | range    @min_max    # stack: node range -> node

range    = SOPEN NUMBER DASH NUMBER SCLOSE @range # stack: number number

IDENTIFIER = NAME !EQUAL
NAME       = < [a-zA-Z_][-a-zA-Z_0-9]* > !':' @identifier -
LABEL      = < [a-zA-Z_][-a-zA-Z_0-9]* > ':'  @label      -
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

-           = (space | comment)*
space       = ' ' | '\t' | '\r\n' | '\n' | '\r'
comment     = '#' ( !end-of-line .)*
end-of-line = '\r\n' | '\n' | '\r'
end-of-file = !.




