
WHAT

This program is called:

"A tool for exploring sound as pure form." or "sound as pure form" or "sapf"

It is an interpreter for a language for creating and transforming sound. The
language is mostly functional, stack based and uses postfix notation similar to
FORTH. It represents audio and control events using lazy, possibly infinite
sequences. It intends to do for lazy sequences what APL does for arrays: provide
very high level functions with pervasive automatic mapping, scanning, and
reduction operators. This makes for a language where short programs can achieve
results out of proportion to their size. Because nearly all of the programmer
accessible data types are immutable, the language can easily run multiple
threads without deadlock or corruption.

WHY

Other languages that inspired this one:
    APL, Joy[1], Haskell, Piccola[2], Nyquist[3], SuperCollider[4].

APL and FORTH (from which Joy derives) are both widely derided for being
write-only languages. Nevertheless, there has yet to be a language of such
concise expressive power as APL or its descendants. APL is powerful not because
of its bizarre symbols or syntax, but due to the way it automatically maps
operations over arrays and allows iterations at depth within arrays. This means
one almost never needs to write a loop or think about operations one-at-a-time.
Instead one can think about operations on whole structures.

Here is a great quote from Alan Perlis[5] on APL, which that also reflects my
interest in this way of programming :

"What attracted me, then, to APL was a feeling that perhaps through APL one
might begin to acquire some of the dimensions in programming that we revere in
natural language — some of the pleasures of composition; of saying things
elegantly; of being brief, poetic, artistic, that makes our natural languages
so precious to us."

The Joy language introduced concatenative functional programming. This generally
means a stack based virtual machine, and a program consisting of words which are
functions taking an input stack and returning an output stack. The natural
syntax that results is postfix. Over a very long time I have come to feel that
syntax gets in between me and the power in a language. Postfix is the least
syntax possible.

There are several reasons I like the concatenative style of programming:
    Function composition is concatenation. 
    Pipelining values through functions to get new values is the most natural
        idiom.
    Functions are applied from left to right instead of inside out. 
    Support for multiple return values comes for free. 
    No need for operator precedence. 
    Fewer delimiters are required: 
        Parentheses are not needed to control operator precedence.
        Semicolons are not needed to separate statements.
        Commas are not needed to separate arguments.

(Note: Sapf is inspired by, but is not purely a concatenative language because it has lexical variables.)

When I am programming interactively, I most often find myself in the situation
where I have a value and I want to transform it to something else. The thing to
do is apply a function with some parameters. With concatenative programming this
is very natural. You string along several words and get a new value.


QUICK SET-UP

Put the sapf program into ~/bin or wherever you keep commands.
Since this binary is unsigned you will need to remove Apple's quarantine attribute:

xattr -r -d com.apple.quarantine <path-to-sapf-binary>

Set up the environment variables in Terminal or your shell profile. 
For example:

export SAPF_HISTORY="$HOME/sapf-files/sapf-history.txt"
export SAPF_LOG="$HOME/sapf-files/sapf-log.txt"
export SAPF_PRELUDE="$HOME/sapf-files/sapf-prelude.txt"
export SAPF_EXAMPLES="$HOME/sapf-files/sapf-examples.txt"
export SAPF_README="$HOME/sapf-files/README.txt"
export SAPF_RECORDINGS="$HOME/sapf-files/recordings"
export SAPF_SPECTROGRAMS="$HOME/sapf-files/spectrograms"

read this README. 

check out some examples:
    start sapf in Terminal.
    open sapf-examples.txt in a text editor
    copy an example onto the command line.


COMMAND LINE

sapf [-r sample-rate][-p prelude-file]

sapf [-h]
    print this help

OPTIONS
    -r sample-rate
        Sets the session sample rate. The default sample rate is 96000 Hz.
        
    -p prelude-file
        The path to a file of code to load before entering the read-eval-print
        loop. If this argument is not supplied, the prelude file is loaded from
        the path stored in the environment variable SAPF_PRELUDE.
        
    -h
        print help for the command line options.

ENVIRONMENT VARIABLES 
    
    SAPF_PRELUDE 
        the path for a code file to be loaded before entering the
        read-eval-print loop.
        
    SAPF_RECORDINGS
        the path where output sound files should be written.
        
    SAPF_SPECTROGRAMS
        the path where spectrogram images should be written.
        
    SAPF_HISTORY
        the path where the command line history is stored for recall at runtime.
        
    SAPF_LOG
        the path where a log of command line inputs are stored for posterity.
        
    SAPF_EXAMPLES
        the path to a file of examples. 

    SAPF_README
        the path to this README file.


BUILT IN FUNCTION HELP
    You can get a list of all defined functions by typing
        "helpall".
    You can get help for a particular built-in function by typing 
        "`someword help" (note the backquote).

A VERY FEW EXAMPLES

    ;; type 'stop' to stop sound playback

    ;; play a sine wave at 800 Hz and initial phase of zero.
    800 0 sinosc .3 * play   
    
    ;; the analog bubbles example from SuperCollider:
    .4 0 lfsaw 2 * [8 7.23] 0 lfsaw .25 * 5/3 + + ohz 0 sinosc .04 * .2 0 4 combn play

    See the "examples" file for more.

TYPES
    "It is better to have 100 functions operate on one data structure 
    than 10 functions on 10 data structures." -- Alan Perlis

    The language has a bare minimum of data types:
        Real - a 64 bit double precision floating point number for quantifying
            things.
        String - a string of characters for naming things.
        List - Ordered lists of values that function as both arrays and lazy
            potentially infinitely long sequences.
        Form - An object that maps symbolic names to values. A form is a
            dictionary with inheritance.
        Function - Functions are values that when applied take values from the
            stack and evaluate an expression.
        Ref - A mutable container for a value. This is the only mutable data
            type.
        
        
SYNTAX
    Expressions are sequences of words (or other syntactic elements) written in postfix 
    form. All words are are executed in left to right order as they are encountered. 
    When a word is executed it looks up the value bound to that word. If the value is a 
    function, then the function is applied and any arguments of the function are taken 
    from the stack. If the value is not a function then the value itself is pushed onto 
    the stack. 

    
    2 3 *  -->  6
    
    "-->" means "evaluates to" throughout this document.
    

COMMENTS
    Comments begin with a semicolon character and continue to the end of a line.
    
    ; this is a comment line

NUMBERS

    Numbers are written in the usual ways.
        1  2.3 .5 7.
        
    Scientific notation
        3.4e-3 1.7e4
        
    Suffixes. There are several suffixes that scale the value.
    
        pi - scales the number by 3.141592653589793238...
             pi can stand alone as well as being a suffix.
             pi  2pi  .5pi  .25pi
            
        M - mega. scales the number by one million 1000000.
            1M .5M
            
        k - kilo. scales the number by 1000.
            4k 1.5k
        
        h - hecto. scales the number by 100.
            8h
            
        c - centi. scales the number by .01
            386c 702c
            
        m - milli. scales the number by .001
            53m 125m
            
        u - micro. scales the number by .000001
            20u
    
    Infix fractions with no intervening spaces are interpreted as literal real 
    numbers. Both the numerator and denominator can be floating point as 
    described above.
    
        5/4  9/7  15/11  pi/4  7.5/4  1k/3
    


STRINGS
    Strings are enclosed in double quotes. 
    
    "This is a string"
    
    \\n is newline and \\t is tab.
    
    "\\tThis string begins with a tab and ends with a newline.\\n"

WORDS
    Words are sequences of characters delimited by spaces, square brackets,
    curly brackets, parentheses, or one of the quote characters described below 
    (except for the equals sign, which can occur in a word).
    
    these are words

    When a word is executed it looks up the value bound to that word. If the
    value is a function, then the function is applied and any arguments of the
    function are taken from the stack. If the value is not a function then the
    value itself is pushed onto the stack. 

QUOTES
    There are certain symbols that, when immediately preceding a word, change
    the normal evaluation behavior. Normally when a word occurs in an
    expression, the value bound to the word symbol is looked up and applied.
    For example, when a word appears by itself like this:
        sin 
    The interpreter looks up the value bound to the 'sin symbol, which is a
    function, and applies it.

    If a word is preceded by backquote, the interpreter looks up the value bound
    to the symbol without applying it.
        `sin
    The function bound to the 'sin symbol will be pushed onto the stack instead
    of being applied.
    
    If a word is preceded by single quote, the interpreter pushes the symbol
    itself onto the stack.
        'sin
    
    If a word is preceded by comma, the interpreter pops the object on top of the
    stack, looks up the value bound to the symbol in the object, and pushes that
    value onto the stack.
        ,name
    
    If a word is preceded by dot, the interpreter looks up the value bound to
    the symbol in the object on top of the stack, and applies it.
        .name
        
    If a word is preceded by the equals sign, the interpreter binds a value to a
    symbol in the current scope. The current scope extends to the next closing
    square bracket. There are no mutable local variables. Symbols once bound may
    not be changed except at the outer-most scope (the workspace). It is not
    possible to bind values to symbols outside the current scope.
        
        123 = x
 
	You can bind multiple values from the stack using parentheses after the = sign.
	This saves having to pop them off and bind them in reverse order.

		1 2 3 = (a b c)    ; is equivalent to:   1 2 3 = c = b = a
		a b c --> 1 2 3

	You can bind values from lists using square brackets after the = sign.

		[1 2 3 4 5] = [a b c]
		a b c --> 1 2 3
	
		#[1 2 3 4 5] = [a b c]  ; also works for signals. Don't use # on the right hand side.
		a b c --> 1 2 3
	
	Don't get clever though:

	[1 2][3 4] = ([a b][c d])  ; syntax error. nested destructuring is not supported.
    
    
FUNCTIONS

    Functions are a backslash followed by a list of argument names, followed by
    a function body within square brackets.
    
    \a b [a b + a b *]
    
    When the interpreter encounters a function in the code, it creates an instance
    of the function that captures the free variables of the function and pushes that
    onto the stack. It does not apply the function.
    A function may be applied using the ! operator.
    
    3 4 \a b [a b + a b *] !  -->  7 12
    
    The function may be assigned to a word.
    
    \a b [a b + a b *] = blub
    3 4 blub  -->  7 12
    
    Optionally, a help string may follow the function arguments.
    
    \a b 
        "(a b --> sum product) returns the sum and product of a and b." 
        [a b + a b *]
    
    Unlike other concatenative languages, the body of a function is executed on 
    an empty stack. Values from the calling stack are not accessible except via 
    the named arguments.
    
LISTS

    Lists are created by writing expressions within square brackets. The
    expressions are evaluated and the items left on the stack become the
    elements of the list.
    
    [1 2 3]
    
    [1 2 + 3 4 *]  -->  [3 12]
    
    [2 aa 3 ba]  -->  [2 3 2]

    There are two types of lists, value lists aka "streams" and numeric lists
    aka "signals". Signals are defined using an initial '#' character as follows.
    
    #[1 2 3]
    
    #[1 2 + 3 4 *]  -->  #[3 12]

FORMS
    Also known as dictionary, map, or record, a Form is a set of bindings from keys 
    to values. Forms may inherit from other forms.
    Forms are enclosed in curly braces. Keys are preceeded by colons.
    
    { :a 1 :b 2 } = x    ; the value 1 is bound to the key a, and the value 2 is 
                         ; bound to the key b.
    
    { x :c 3 } = y       ; y inherits from x and binds the value 3 to the key c.
    
    The position of keys is actually completely arbitrary within the braces. It is 
    only required that the number of values pushed onto the stack equals, or in the 
    case of inheritance exceeds by one, the number of keys.
    
    So the following are completely equivalent to the above:
    
    {1 2 :a :b} = x
    {x 3 :c} = y

    {:a :b 1 2} = x
    {:c x 3} = y

    This can be useful when returning multiple values from functions and binding 
    them to multiple keys. In general, I would recommend the first syntax above for 
    most cases.
    
    Multiple inheritance is supported by specifying a list of parents.
    Multiple inheritance is an experimental feature and may be removed since it
    prevents certain optimizations that may prove more useful.
    Parent objects are ordered according to an algorithm defined for
    the Dylan programming language[6].

    {:a 1} = a
    {a :b 2} = b
    {a :c 3} = c
    {[b c] :d 4} = d        ; inherit first from b then from c.
    d --> {{{{:a 1}  :c 3}  :b 2}  :d 4}   ; b is reached before c in traversal.
                                           ; a is inherited from only once.

AUTO-MAPPING

    Many built-in operators which take a scalar argument will automatically map
    over a signal or stream passed in that argument position. For example, the
    "to" operator returns a sequence from a starting number to an ending number:
    
        0 4 to  -->  [0 1 2 3 4]
        
    If a list is passed as one of the arguments, then that list is auto-mapped
    and a list of lists is returned.
    
        [0 2] 4 to  -->  [[0 1 2 3 4][2 3 4]]
        
        0 [2 3 4] to  -->  [[0 1 2][0 1 2 3][0 1 2 3 4]]
        
    If lists are passed in for multiple arguments that are subject to
    auto-mapping, then they will be auto-mapped with successive values from each
    of the arguments.
    
        [0 7][2 9] to  -->  [[0 1 2][7 8 9]]
        
    When multiple arguments are auto-mapped, the result will be of the same
    length as the shortest list.
    
        [0 1][5 4 3] to  -->  [[0 1 2 3 4 5][1 2 3 4]]
        
    Auto-mapping may be performed over infinite lists. ord is a function which 
    returns an infinite list of integers starting with 1. 
        ord --> [1 2 3 4 5 ...]
    
        0 ord to  -->  [[0 1][0 1 2][0 1 2 3][0 1 2 3 4][0 1 2 3 4 5]...]

THE "EACH" OPERATOR

    Sometimes an operator needs to be applied at a deeper level than the top
    level. The @ sign, known as the "each" operator, tags the top value on the
    stack so that the next function that consumes it will operate over each of
    its values instead of the list as a whole.
    
    For example say we have the following nested list:
    
        [[1 2 3] [4 5 6]]
        
    If we reverse it we get the outer list reversed:
    
        [[1 2 3] [4 5 6]] reverse  -->  [[4 5 6] [1 2 3]]
        
    What if we want to reverse each of the inner lists? We use the each
    operator:
    
        [[1 2 3] [4 5 6]] @ reverse  -->  [[3 2 1] [6 5 4]]
    
    We can use the each operator to do outer products. Normally math operators
    proceed over lists element-wise like so:
    
        [1 2][10 20] +   -->   [11 22]
    
    If we use the each operator we can apply + to each element of one list and
    the whole other list.
    
        [1 2] @ [10 20] +  -->  [[11 21] [12 22]]
        
        [1 2] [10 20] @ +  -->  [[11 12] [21 22]]
        
    This works because math operators auto-map over lists. 
    Other operators do not auto-map over lists, for example the 2ple operator.
    2ple creates a two item list from the two items on the top of the stack.
    
        [1 2] [10 20] 2ple  -->  [[1 2] [10 20]] 

        [1 2] @ [10 20] 2ple  -->  [[1 [10 20]] [2 [10 20]]]
        
        [1 2] [10 20] @ 2ple  -->  [[[1 2] 10] [[1 2] 20]]
        
        [1 2] @ [10 20] @ 2ple --> [[1 10] [2 20]]
        
    In order to do an outer product we need to use ordered each operators. These
    perform nested loops.
    
        [1 2] @1 [10 20] @2 2ple  -->  [[[1 10] [1 20]] [[2 10] [2 20]]]
        
        [1 2] @2 [10 20] @1 2ple  -->  [[[1 10] [2 10]] [[1 20] [2 20]]]
        
        
    You can do mapping two (or more) levels deep with @@ (or @@@, @@@@, etc) :
    
        [[[1 2 3] [4 5]] [[6 7] [8 9 10]]] @@ reverse  
            -->  [[[3 2 1] [5 4]] [[7 6] [10 9 8]]]
    
    ord @1 ord @2 to   --> an infinite list of infinite lists of finite lists:
        [
            [[1] [1 2] [1 2 3] [1 2 3 4] [1 2 3 4 5] ...] 
            [[2 1] [2] [2 3] [2 3 4] [2 3 4 5] ...] 
            [[3 2 1] [3 2] [3] [3 4] [3 4 5] ...] 
            [[4 3 2 1] [4 3 2] [4 3] [4] [4 5] ...] 
            [[5 4 3 2 1] [5 4 3 2] [5 4 3] [5 4] [5] ...] 
            ...
        ]

    Lists of Forms can be constructed using the each operator.
    
    {:a ord @ :b 0}  -->  [{:a 1 :b 0} {:a 2 :b 0} {:a 3 :b 0} {:a 4 :b 0} ...]
    
    {:a 1 3 to @1  :b 1 4 to @2}  -->          ; outer product
        [
            [{:a 1  :b 1} {:a 1  :b 2} {:a 1  :b 3} {:a 1  :b 4}]
            [{:a 2  :b 1} {:a 2  :b 2} {:a 2  :b 3} {:a 2  :b 4}]
            [{:a 3  :b 1} {:a 3  :b 2} {:a 3  :b 3} {:a 3  :b 4}]
        ]
        
    Lists of lists can be created using the each operator within the list
    constructor syntax:
    
    [[1 2 3] @ 4 5] --> [[1 4 5] [2 4 5] [3 4 5]]
    
    [[1 2 3] @1 [4 5 6] @2] --> 
            [[[1 4] [1 5] [1 6]] [[2 4] [2 5] [2 6]] [[3 4] [3 5] [3 6]]]

MULTI-CHANNEL EXPANSION

    Multi-channel expansion is a kind of auto-mapping for operators that process
    signals and not streams (for example, unit generators). These operators auto-map
    over streams, but not signals.
    
    ; the saw oscillator expands over the list [300 301] to produce stereo
    ; channels that beat with each other at 1 Hz.
    [300 301] 0 saw .3 * play
    
REDUCING AND SCANNING MATH OPERATORS

    For all two argument math operators, adding a forward slash after the
    operator turns it into a list reducing operator and adding a backward slash
    turns it into a list scanning operator.
    
    1 2 + --> 3   normal addition
    [1 2 3 4] +/  --> 10    sum
    [1 2 3 4] +\  --> [1 3 6 10]   accumulation
    
    [1 2 3 4] */  --> 24    product
    [1 2 3 4] *\  --> [1 2 6 24]   scan of multiplication
    
    Adding a caret after the operator makes it work in pair-wise fashion. It
    outputs the first value, then outputs the operator applied to successive
    pairs.
    [1 2 3 4 5 6] +^     --> [1 3 5 7 9 11]
    
    -^ and +\ are inverses of each other.
    [7 9 16 20 1 5] -^   --> [7 2 7 4 -19 4]        pairwise difference
    [7 2 7 4 -19 4] +\   --> [7 9 16 20 1 5]        accumulation


REFERENCES    

1. http://www.kevinalbrecht.com/code/joy-mirror/joy.html

2. http://scg.unibe.ch/research/piccola

3. http://www.cs.cmu.edu/~music/nyquist/

4. http://supercollider.sourceforge.net

5. http://www.jsoftware.com/papers/perlis78.htm

6. http://haahr.tempdomainname.com/dylan/linearization-oopsla96.html

