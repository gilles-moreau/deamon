/*****************************************************************************
 *  list.c
 *****************************************************************************
 *  Copyright (C) 2001-2002 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Chris Dunlap <cdunlap@llnl.gov>.
 *
 *  This file is from LSD-Tools, the LLNL Software Development Toolbox.
 *
 *  LSD-Tools is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  LSD-Tools is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 *  more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with LSD-Tools; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
 *****************************************************************************
 *  Refer to "list.h" for documentation on public functions.
 *****************************************************************************/

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#include "src/common/list.h"
#include "src/common/logs.h"
#include "src/common/macros.h"

/***************
 *  Constants  *
 ***************/

#define list_alloc() malloc(sizeof(struct xlist))
#define list_free(_l) free(l)
#define list_node_alloc() malloc(sizeof(struct listNode))
#define list_node_free(_p) free(_p)
#define list_iterator_alloc() malloc(sizeof(struct listIterator))
#define list_iterator_free(_i) free(_i)

/****************
 *  Data Types  *
 ****************/

struct listNode {
	void                 *data;         /* node's data                       */
	struct listNode      *next;         /* next node in list                 */
};

struct listIterator {
	struct xlist         *list;         /* the list being iterated           */
	struct listNode      *pos;          /* the next node to be iterated      */
	struct listNode     **prev;         /* addr of 'next' ptr to prv It node */
	struct listIterator  *iNext;        /* iterator chain for list_destroy() */
};

struct xlist {
	struct listNode      *head;         /* head of the list                  */
	struct listNode     **tail;         /* addr of last node's 'next' ptr    */
	struct listIterator  *iNext;        /* iterator chain for list_destroy() */
	ListDelF              fDel;         /* function to delete node data      */
	int                   count;        /* number of nodes in list           */
	pthread_mutex_t       mutex;        /* mutex to protect access to list   */
};

typedef struct listNode * ListNode;


/****************
 *  Prototypes  *
 ****************/

static void *_list_node_create(List l, ListNode *pp, void *x);
static void *_list_node_destroy(List l, ListNode *pp);
static void *_list_pop_locked(List l);
static void *_list_append_locked(List l, void *x);

static int _list_mutex_is_locked(pthread_mutex_t *mutex);

/***************
 *  Functions  *
 ***************/

/* list_create()
 */
List
list_create (ListDelF f)
{
	List l = list_alloc();

	l->head = NULL;
	l->tail = &l->head;
	l->iNext = NULL;
	l->fDel = f;
	l->count = 0;
	skrum_mutex_init(&l->mutex);

	return l;
}

/* list_destroy()
 */
void
list_destroy (List l)
{
	ListIterator i, iTmp;
	ListNode p, pTmp;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);

	i = l->iNext;
	while (i) {
		iTmp = i->iNext;
		list_iterator_free(i);
		i = iTmp;
	}
	p = l->head;
	while (p) {
		pTmp = p->next;
		if (p->data && l->fDel)
			l->fDel(p->data);
		list_node_free(p);
		p = pTmp;
	}
	skrum_mutex_unlock(&l->mutex);
	skrum_mutex_destroy(&l->mutex);
	list_free(l);
}

/* list_is_empty()
 */
int
list_is_empty (List l)
{
	int n;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);
	n = l->count;
	skrum_mutex_unlock(&l->mutex);

	return (n == 0);
}

/*
 * Return the number of items in list [l].
 * If [l] is NULL, return 0.
 */
int list_count(List l)
{
	int n;

	if (!l)
		return 0;

	skrum_mutex_lock(&l->mutex);
	n = l->count;
	skrum_mutex_unlock(&l->mutex);

	return n;
}

List list_shallow_copy(List l)
{
	List m = list_create(NULL);
	ListNode p;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);
	skrum_mutex_lock(&m->mutex);

	p = l->head;
	while (p) {
		_list_append_locked(m, p->data);
		p = p->next;
	}

	skrum_mutex_unlock(&m->mutex);
	skrum_mutex_unlock(&l->mutex);
	return m;
}

/* list_append()
 */
void *
list_append (List l, void *x)
{
	void *v;

	assert(l != NULL);
	assert(x != NULL);
	skrum_mutex_lock(&l->mutex);
	v = _list_append_locked(l, x);
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_append_list()
 */
int
list_append_list (List l, List sub)
{
	ListIterator itr;
	void *v;
	int n = 0;

	assert(l != NULL);
	assert(l->fDel == NULL);
	assert(sub != NULL);
	itr = list_iterator_create(sub);
	while((v = list_next(itr))) {
		if (list_append(l, v))
			n++;
		else
			break;
	}
	list_iterator_destroy(itr);

	return n;
}

/*
 *  Pops off list [sub] to [l] with maximum number of entries.
 *  Set max = 0 to transfer all entries.
 *  Note: list [l] must have the same destroy function as list [sub].
 *  Note: list [sub] may be returned empty, but not destroyed.
 *  Returns a count of the number of items added to list [l].
 */
int list_transfer_max(List l, List sub, int max)
{
	void *v;
	int n = 0;

	assert(l);
	assert(sub);
	assert(l->fDel == sub->fDel);

	while ((!max || n <= max) && (v = list_pop(sub))) {
		list_append(l, v);
		n++;
	}

	return n;
}

/*
 *  Pops off list [sub] to [l].
 *  Set max = 0 to transfer all entries.
 *  Note: list [l] must have the same destroy function as list [sub].
 *  Note: list [sub] will be returned empty, but not destroyed.
 *  Returns a count of the number of items added to list [l].
 */
int list_transfer(List l, List sub)
{
	return list_transfer_max(l, sub, 0);
}

/* list_prepend()
 */
void *
list_prepend (List l, void *x)
{
	void *v;

	assert(l != NULL);
	assert(x != NULL);
	skrum_mutex_lock(&l->mutex);

	v = _list_node_create(l, &l->head, x);
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_find_first()
 */
void *
list_find_first (List l, ListFindF f, void *key)
{
	ListNode p;
	void *v = NULL;

	assert(l != NULL);
	assert(f != NULL);
	assert(key != NULL);
	skrum_mutex_lock(&l->mutex);

	for (p = l->head; p; p = p->next) {
		if (f(p->data, key)) {
			v = p->data;
			break;
		}
	}
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_remove_first()
 */
void *
list_remove_first (List l, ListFindF f, void *key)
{
	ListNode *pp;
	void *v = NULL;

	assert(l != NULL);
	assert(f != NULL);
	assert(key != NULL);
	skrum_mutex_lock(&l->mutex);

	pp = &l->head;
	while (*pp) {
		if (f((*pp)->data, key)) {
			v = _list_node_destroy(l, pp);
			break;
		} else {
			pp = &(*pp)->next;
		}
	}
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_delete_all()
 */
int
list_delete_all (List l, ListFindF f, void *key)
{
	ListNode *pp;
	void *v;
	int n = 0;

	assert(l != NULL);
	assert(f != NULL);
	skrum_mutex_lock(&l->mutex);

	pp = &l->head;
	while (*pp) {
		if (f((*pp)->data, key)) {
			if ((v = _list_node_destroy(l, pp))) {
				if (l->fDel)
					l->fDel(v);
				n++;
			}
		}
		else {
			pp = &(*pp)->next;
		}
	}
	skrum_mutex_unlock(&l->mutex);

	return n;
}

/* list_for_each()
 */
int
list_for_each (List l, ListForF f, void *arg)
{
	int max = -1;	/* all values */
	return list_for_each_max(l, &max, f, arg, 1);
}

int list_for_each_nobreak(List l, ListForF f, void *arg)
{
	int max = -1;	/* all values */
	return list_for_each_max(l, &max, f, arg, 0);
}

int list_for_each_max(List l, int *max, ListForF f, void *arg,
		      int break_on_fail)
{
	ListNode p;
	int n = 0;
	bool failed = false;

	assert(l != NULL);
	assert(f != NULL);
	skrum_mutex_lock(&l->mutex);

	for (p = l->head; (*max == -1 || n < *max) && p; p = p->next) {
		n++;
		if (f(p->data, arg) < 0) {
			failed = true;
			if (break_on_fail)
				break;
		}
	}
	*max = l->count - n;
	skrum_mutex_unlock(&l->mutex);

	if (failed)
		n = -n;

	return n;
}

/* list_flush()
 */
int list_flush(List l) {

	ListNode *pp;
	void *v;
	int n = 0;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);

	pp = &l->head;
	while (*pp) {
		if ((v = _list_node_destroy(l, pp))) {
			if (l->fDel)
				l->fDel(v);
			n++;
		}
	}
	skrum_mutex_unlock(&l->mutex);

	return n;
}

/* list_push()
 */
void *
list_push (List l, void *x)
{
	void *v;

	assert(l != NULL);
	assert(x != NULL);
	skrum_mutex_lock(&l->mutex);

	v = _list_node_create(l, &l->head, x);
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/*
 * Handle translation between ListCmpF and signature required by qsort.
 * glibc has this as __compar_fn_t, but that's non-standard so we define
 * our own instead.
 */
typedef int (*ConstListCmpF) (__const void *, __const void *);

/* list_sort()
 *
 * This function uses the libC qsort().
 *
 */
void
list_sort(List l, ListCmpF f)
{
	char **v;
	int n;
	int lsize;
	void *e;
	ListIterator i;

	assert(l != NULL);
	assert(f != NULL);
	skrum_mutex_lock(&l->mutex);

	if (l->count <= 1) {
		skrum_mutex_unlock(&l->mutex);
		return;
	}

	lsize = l->count;
	v = malloc(lsize * sizeof(char *));

	n = 0;
	while ((e = _list_pop_locked(l))) {
		v[n] = e;
		++n;
	}

	qsort(v, n, sizeof(char *), (ConstListCmpF)f);

	for (n = 0; n < lsize; n++) {
		_list_append_locked(l, v[n]);
	}

	free(v);

	/* Reset all iterators on the list to point
	 * to the head of the list.
	 */
	for (i = l->iNext; i; i = i->iNext) {
		i->pos = i->list->head;
		i->prev = &i->list->head;
	}

	skrum_mutex_unlock(&l->mutex);
}

/* list_pop()
 */
void *
list_pop (List l)
{
	void *v;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);

	v = _list_pop_locked(l);
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_peek()
 */
void *
list_peek (List l)
{
	void *v;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);

	v = (l->head) ? l->head->data : NULL;
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_enqueue()
 */
void *
list_enqueue (List l, void *x)
{
	void *v;

	assert(l != NULL);
	assert(x != NULL);
	skrum_mutex_lock(&l->mutex);

	v = _list_node_create(l, l->tail, x);
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_dequeue()
 */
void *
list_dequeue (List l)
{
	void *v;

	assert(l != NULL);
	skrum_mutex_lock(&l->mutex);

	v = _list_node_destroy(l, &l->head);
	skrum_mutex_unlock(&l->mutex);

	return v;
}

/* list_iterator_create()
 */
ListIterator
list_iterator_create (List l)
{
	ListIterator i;

	assert(l != NULL);
	i = list_iterator_alloc();

	i->list = l;
	skrum_mutex_lock(&l->mutex);

	i->pos = l->head;
	i->prev = &l->head;
	i->iNext = l->iNext;
	l->iNext = i;

	skrum_mutex_unlock(&l->mutex);

	return i;
}

/* list_iterator_reset()
 */
void
list_iterator_reset (ListIterator i)
{
	assert(i != NULL);
	skrum_mutex_lock(&i->list->mutex);

	i->pos = i->list->head;
	i->prev = &i->list->head;

	skrum_mutex_unlock(&i->list->mutex);
}

/* list_iterator_destroy()
 */
void
list_iterator_destroy (ListIterator i)
{
	ListIterator *pi;

	assert(i != NULL);
	skrum_mutex_lock(&i->list->mutex);

	for (pi = &i->list->iNext; *pi; pi = &(*pi)->iNext) {
		if (*pi == i) {
			*pi = (*pi)->iNext;
			break;
		}
	}
	skrum_mutex_unlock(&i->list->mutex);

	list_iterator_free(i);
}

/* list_next()
 */
void *
list_next (ListIterator i)
{
	ListNode p;

	assert(i != NULL);
	skrum_mutex_lock(&i->list->mutex);

	if ((p = i->pos))
		i->pos = p->next;
	if (*i->prev != p)
		i->prev = &(*i->prev)->next;

	skrum_mutex_unlock(&i->list->mutex);

	return (p ? p->data : NULL);
}

/* list_peek_next()
 */
void *
list_peek_next (ListIterator i)
{
	ListNode p;

	assert(i != NULL);
	skrum_mutex_lock(&i->list->mutex);

	p = i->pos;

	skrum_mutex_unlock(&i->list->mutex);

	return (p ? p->data : NULL);
}

/* list_insert()
 */
void *
list_insert (ListIterator i, void *x)
{
	void *v;

	assert(i != NULL);
	assert(x != NULL);
	skrum_mutex_lock(&i->list->mutex);

	v = _list_node_create(i->list, i->prev, x);
	skrum_mutex_unlock(&i->list->mutex);

	return v;
}

/* list_find()
 */
void *
list_find (ListIterator i, ListFindF f, void *key)
{
	void *v;

	assert(i != NULL);
	assert(f != NULL);
	assert(key != NULL);

	while ((v = list_next(i)) && !f(v,key)) {;}

	return v;
}

/* list_remove()
 */
void *
list_remove (ListIterator i)
{
	void *v = NULL;

	assert(i != NULL);
	skrum_mutex_lock(&i->list->mutex);

	if (*i->prev != i->pos)
		v = _list_node_destroy(i->list, i->prev);
	skrum_mutex_unlock(&i->list->mutex);

	return v;
}

/* list_delete_item()
 */
int
list_delete_item (ListIterator i)
{
	void *v;

	assert(i != NULL);

	if ((v = list_remove(i))) {
		if (i->list->fDel)
			i->list->fDel(v);
		return 1;
	}

	return 0;
}

/*
 * Inserts data pointed to by [x] into list [l] after [pp],
 * the address of the previous node's "next" ptr.
 * Returns a ptr to data [x], or NULL if insertion fails.
 * This routine assumes the list is already locked upon entry.
 */
static void *_list_node_create(List l, ListNode *pp, void *x)
{
	ListNode p;
	ListIterator i;

	assert(l != NULL);
	assert(_list_mutex_is_locked(&l->mutex));
	assert(pp != NULL);
	assert(x != NULL);

	p = list_node_alloc();

	p->data = x;
	if (!(p->next = *pp))
		l->tail = &p->next;
	*pp = p;
	l->count++;

	for (i = l->iNext; i; i = i->iNext) {
		if (i->prev == pp)
			i->prev = &p->next;
		else if (i->pos == p->next)
			i->pos = p;
		assert((i->pos == *i->prev) ||
		       ((*i->prev) && (i->pos == (*i->prev)->next)));
	}

	return x;
}

/*
 * Removes the node pointed to by [*pp] from from list [l],
 * where [pp] is the address of the previous node's "next" ptr.
 * Returns the data ptr associated with list item being removed,
 * or NULL if [*pp] points to the NULL element.
 * This routine assumes the list is already locked upon entry.
 */
static void *_list_node_destroy(List l, ListNode *pp)
{
	void *v;
	ListNode p;
	ListIterator i;

	assert(l != NULL);
	assert(_list_mutex_is_locked(&l->mutex));
	assert(pp != NULL);

	if (!(p = *pp))
		return NULL;

	v = p->data;
	if (!(*pp = p->next))
		l->tail = pp;
	l->count--;

	for (i = l->iNext; i; i = i->iNext) {
		if (i->pos == p)
			i->pos = p->next, i->prev = pp;
		else if (i->prev == &p->next)
			i->prev = pp;
		assert((i->pos == *i->prev) ||
		       ((*i->prev) && (i->pos == (*i->prev)->next)));
	}
	list_node_free(p);

	return v;
}

#ifndef NDEBUG
static int
_list_mutex_is_locked (pthread_mutex_t *mutex)
{
/*  Returns true if the mutex is locked; o/w, returns false.
 */
	int rc;

	assert(mutex != NULL);
	rc = pthread_mutex_trylock(mutex);
	return(rc == EBUSY ? 1 : 0);
}
#endif /* !NDEBUG */

/* _list_pop_locked
 *
 * Pop an item from the list assuming the
 * the list is already locked.
 */
static void *
_list_pop_locked(List l)
{
	void *v;

	v = _list_node_destroy(l, &l->head);

	return v;
}

/* _list_append_locked()
 *
 * Append an item to the list. The function assumes
 * the list is already locked.
 */
static void *
_list_append_locked(List l, void *x)
{
	void *v;

	v = _list_node_create(l, l->tail, x);

	return v;
}