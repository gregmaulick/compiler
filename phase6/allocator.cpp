/*
 * File:	allocator.cpp
 *
 * Description:	This file contains the member function definitions for
 *		functions dealing with storage allocation.  The actual
 *		classes are declared elsewhere, mainly in Tree.h.
 *
 *		Extra functionality:
 *		- maintaining minimum offset in nested blocks
 *		- allocation within while, for, and if-then-else statements
 */

# include <cassert>
# include <iostream>
# include "checker.h"
# include "machine.h"
# include "tokens.h"
# include "Tree.h"

using namespace std;


/*
 * Function:	Type::size
 *
 * Description:	Return the size of a type in bytes.
 */

unsigned Type::size() const
{
    unsigned count;


    assert(_declarator != FUNCTION && _declarator != ERROR);
    count = (_declarator == ARRAY ? _length : 1);

    if (_indirection > 0)
	return count * SIZEOF_PTR;

    if (_specifier == DOUBLE)
	return count * SIZEOF_DOUBLE;

    if (_specifier == INT)
	return count * SIZEOF_INT;

    if (_specifier == CHAR)
	return count * SIZEOF_CHAR;

    return 0;
}


/*
 * Function:	Block::allocate
 *
 * Description:	Allocate storage for this block.  We assign decreasing
 *		offsets for all symbols declared within this block, and
 *		then for all symbols declared within any nested block.
 *		Only symbols that have not already been allocated an
 *		offset will be assigned one, since the parameters are
 *		already assigned special offsets.
 */

void Block::allocate(int &offset) const
{
    int temp, saved;
    unsigned i;
    Symbols symbols;


    symbols = _decls->symbols();

    for (i = 0; i < symbols.size(); i ++)
	if (symbols[i]->offset == 0) {
	    offset -= symbols[i]->type().size();
	    symbols[i]->offset = offset;
	}

    saved = offset;

    for (i = 0; i < _stmts.size(); i ++) {
	temp = saved;
	_stmts[i]->allocate(temp);
	offset = min(offset, temp);
    }
}


/*
 * Function:	While::allocate
 *
 * Description:	Allocate storage for this while statement, which
 *		essentially means allocating storage for variables declared
 *		as part of its statement.
 */

void While::allocate(int &offset) const
{
    _stmt->allocate(offset);
}


/*
 * Function:	For::allocate
 *
 * Description:	Allocate storage for this for statement, which
 *		essentially means allocating storage for variables declared
 *		as part of its statement.
 */

void For::allocate(int &offset) const
{
    _stmt->allocate(offset);
}


/*
 * Function:	If::allocate
 *
 * Description:	Allocate storage for this if-then or if-then-else
 *		statement, which essentially means allocating storage for
 *		variables declared as part of its statements.
 */

void If::allocate(int &offset) const
{
    int saved, temp;


    saved = offset;
    _thenStmt->allocate(offset);

    if (_elseStmt != nullptr) {
	temp = saved;
	_elseStmt->allocate(temp);
	offset = min(offset, temp);
    }
}


/*
 * Function:	Function::allocate
 *
 * Description:	Allocate storage for this function and return the number of
 *		bytes required.  The parameters are allocated offsets as
 *		well, starting with the given offset.
 */

void Function::allocate(int &offset) const
{
    Parameters *params;
    Symbols symbols;


    params = _id->type().parameters();
    symbols = _body->declarations()->symbols();

    for (unsigned i = 0; i < params->types.size(); i ++) {
	symbols[i]->offset = offset;
	offset += params->types[i].promote().size();
    }

    offset = 0;
    _body->allocate(offset);
}
