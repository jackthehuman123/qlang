query       := or_expr
or_expr     := and_expr ( "OR" and_expr )*
and_expr    := unary ( "AND"? unary )*
unary       := "NOT" unary | primary
primary     :=  "(" query ")" | predicate
predicate   := IDENT op value
op          := ":" | ":>" | ":<" | ":>=" | ":<=" | ":!" | "~"
value       := NUMBER | STRING | IDENT
        i.e.    10      "foo"     foo 
How to read it: 
`:=`    -> is defined as
`|`     -> or
`( )*`  -> zero or more repititions
`EX`    -> capitals are toekn types

Line 1 -> A query contains an or_expr
Line 2 -> An or_expr contains and_expr ( OR and_expr )
Line n -> LEFT contains RIGHT ( alongside attributes of RIGHT )