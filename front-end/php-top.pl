% $Id$ -*-Prolog-*-

% -----------------------------------------------------------------------------
%  GPROLOG-PHP is Copyright (C) 2002 Salvador Abreu
%  
%     This program is free software; you can redistribute it and/or
%     modify it under the terms of the GNU General Public License as
%     published by the Free Software Foundation; either version 2, or
%     (at your option) any later version.
%  
%     This program is distributed in the hope that it will be useful,
%     but WITHOUT ANY WARRANTY; without even the implied warranty of
%     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
%     General Public License for more details.
%  
%     You should have received a copy of the GNU General Public License
%     along with this program; if not, write to the Free Software
%     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
%     02111-1307, USA.
%  
%  On Debian GNU/Linux systems, the complete text of the GNU General
%  Public License can be found in `/usr/share/common-licenses/GPL'.
% -----------------------------------------------------------------------------

% -----------------------------------------------------------------------------
% ISCO PHP top-level for GNU Prolog.
% -----------------------------------------------------------------------------

:- dynamic(php_term_expansion/2).

top_level :- init_go_php_top_level.

init_go_php_top_level :-
	repeat,
	  g_assign(level, 0),
	  catch(php_top_level, ERROR,
	  	( format('~cerror ~w~n', [1, ERROR]),
		  flush_output )),
	  fail.


% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

php_add_processor(PREDICATE) :-
	GOAL =.. [PREDICATE, ARGin, ARGout],
	assertz((php_term_expansion(ARGin, ARGout) :- GOAL)).

php_delete_processor(PREDICATE) :-
	GOAL =.. [PREDICATE, ARGin, ARGout],
	retract((php_delete_processor(ARGin, ARGout) :- GOAL)).

% - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

php_top_level :-
	php_read(GOAL, VL, Vs), !,
	php_query(GOAL, VL, Vs).

% -----------------------------------------------------------------------------

php_query(GOAL, VL, Vs) :-
	g_read(level, L0), LEVEL is L0+1, g_assign(level, LEVEL),
	( php_term_expansion(GOAL, REAL_GOAL) -> true ; REAL_GOAL=GOAL ),
	flush_output,
	catch(call(REAL_GOAL), ERROR, true),
	flush_output,
	( var(ERROR) ->
	    format("~cok ~w~n", [1, LEVEL]),
	    php_show(VL, Vs),
	    flush_output, 
	    php_what_next
	;
	    format_to_codes(MSG, "~w~n", [ERROR]),
	    length(MSG, LENGTH),
	    format("error ~w ~w ~s", [LEVEL, LENGTH, MSG]),
	    flush_output ),
	g_assign(level, L0), !,
	fail.

php_query(_, _, _) :-
	g_read(level, LEVEL), L0 is LEVEL-1, g_assign(level, L0),
	flush_output,
	format("~cno ~w~n", [1, LEVEL]),
	flush_output.



php_what_next :-
	php_read_token(ANSWER),
	( ANSWER = done -> true
	; ANSWER = relax -> php_what_next
	; ANSWER = more -> fail
	; ANSWER = quit -> php_halt
	; ANSWER = end_of_file -> php_halt
	; ANSWER = query ->
	    php_read_query(GOAL, VL, Vs),
	    ( php_query(GOAL, VL, Vs) ; true ),
	    php_what_next
	; true ).		% take errors to be like "done"


php_read(GOAL, VL, Vs) :- php_read(GOAL, VL, Vs, _).
php_read(GOAL, VL, Vs, ERROR) :-
	flush_output,
	read_token(ACTION),
	( ACTION=query -> php_read_query(GOAL, VL, Vs)
	; ACTION=quit -> php_halt
	; ACTION=punct(end_of_file) -> php_halt
%%%	; ACTION=done -> php_read(GOAL, VL, VS, ERROR)
	; var(ERROR) ->
	    g_read(level, LEVEL),
	    FMT='expected "query" or "quit" (got "~q")~n',
	    format_to_codes(MSG, FMT, [ACTION]),
	    length(MSG, LENGTH),
	    format("~cerror ~w ~w ~s", [1, LEVEL, LENGTH, MSG]),
	    flush_output,
	    php_read(GOAL, VL, Vs, error)
	; true -> php_read(GOAL, VL, Vs, error) ).


php_read_query(GOAL, VL, Vs) :-
	flush_output,
	read_term(XVL, [variable_names(Vs)]),
	( XVL = GOAL/VL, var(VL) -> php_all_names(Vs, VL)
	; XVL = GOAL/VL -> true
	; XVL = GOAL -> php_all_names(Vs, VL) ).


php_read_token(ANSWER) :-
	flush_output,
%	catch(read_token(ANSWER), _, ANSWER=done).
	read_token(ANSWER).


% -----------------------------------------------------------------------------

php_output(FMT) :- php_output(FMT, []).
php_output(FMT, ARGS) :-
	format("~?~n", [FMT | ARGS]),
	flush_output.

php_show_error(ERR):-
	format('~w~n', [ERR]),
	flush_output.


php_all_names([], []).
php_all_names([N=V|Vs], [V>N|VL]) :- php_all_names(Vs, VL).


php_show([], _) :-
	format("end~n", []),
	flush_output.
php_show([V|Vs], LNs) :-
	var(V),
	!,
	php_show_find_name(LNs, V, N),
	format("var ~w unbound~n", [N]),
	flush_output,
	php_show(Vs, LNs).
php_show([N=V|Vs], LNs) :- !, php_show([V>N|Vs], LNs).
php_show([V>N|Vs], LNs) :- 
	integer(V),
	!,
	format("var ~w int ~w~n", [N, V]),
	flush_output,
	php_show(Vs, LNs).
php_show([V>N|Vs], LNs) :- 
	number(V),
	!,
	format("var ~w float ~w~n", [N, V]),
	flush_output,
	php_show(Vs, LNs).
php_show([V>N|Vs], LNs) :-
	!,
	write_term_to_codes(STRING, V, []),
	length(STRING, L),
	format("var ~w string ~w ~s~n", [N, L, STRING]),
	flush_output,
	php_show(Vs, LNs).
php_show([V|Vs], LNs) :-
	php_show_find_name(LNs, V, N),
	php_show([V>N|Vs], LNs).


php_show_find_name([N=V|_], VV, N) :- V==VV, !.
php_show_find_name([_|NVs], VV, N) :- php_show_find_name(NVs, VV, N).

% -----------------------------------------------------------------------------

php_halt :-
	halt(0).

% -----------------------------------------------------------------------------

% $Log$
% Revision 1.1  2003/01/07 19:52:14  spa
% Initial revision
%

% Local Variables:
% mode: prolog
% mode: font-lock
% End:
