# $Id$

# -----------------------------------------------------------------------------
#  GPROLOG-PHP is Copyright (C) 2001 João Conceição and Salvador Abreu
#  GPROLOG-PHP is Copyright (C) 2002 Salvador Abreu
#  
#     This program is free software; you can redistribute it and/or
#     modify it under the terms of the GNU General Public License as
#     published by the Free Software Foundation; either version 2, or
#     (at your option) any later version.
#  
#     This program is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#     General Public License for more details.
#  
#     You should have received a copy of the GNU General Public License
#     along with this program; if not, write to the Free Software
#     Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#     02111-1307, USA.
#  
#  On Debian GNU/Linux systems, the complete text of the GNU General
#  Public License can be found in `/usr/share/common-licenses/GPL'.
# -----------------------------------------------------------------------------

SHELL = /bin/bash

GPLC = gplc${variant}
PROLOG = gprolog${variant}
INSTALL = install

PREFIX = /usr/local
DESTDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib/isco

PHP_FLAGS = --no-debugger --no-top-level

all: php-top.o

clean:
	rm -f *.o

php-%: %.o $(PHP_OBJECTS) $(DB)
	$(GPLC) -o $@ \
		$< \
		$(PHP_OBJECTS) \
		$(PHP_FLAGS) \
		$(DB) $(LIBDB)

%.o:: %.pl
	$(GPLC) --no-redef-error -c $<

# $Log$
# Revision 1.1  2003/01/07 19:52:14  spa
# Initial revision
#

# Local variables:
# mode: font-lock
# End:
