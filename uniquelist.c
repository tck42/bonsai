#include "uniquelist.h"
#include "utils.h"

static void
UniqueList_dealloc(UniqueList *self) {
	Py_TYPE(self)->tp_free((PyObject*)self);
}

/*	Create a new UniqueList object. */
static PyObject *
UniqueList_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	UniqueList *self;

	self = (UniqueList *)PyList_Type.tp_new(type, args, kwds);
	if (self == NULL) return NULL;

	return (PyObject *)self;
}

/*	Convert `list` object to a tuple, which contains only lowercase Python string elements. */
static PyObject *
get_lowercase_tuple(PyObject *list) {
	char *str;
	Py_ssize_t i, n;
	PyObject *tup = NULL;
	PyObject *seq, *item;

	if (list == NULL) return NULL;

	seq = PySequence_Fast(list, "Argument is not iterable.");
	n = PySequence_Fast_GET_SIZE(seq);
	tup = PyTuple_New(n);
	if (tup == NULL) return PyErr_NoMemory();

	for (i = 0; i < n; i++) {
		item = PySequence_Fast_GET_ITEM(seq, i);
		str = lowercase(PyObject2char(item));
		if (PyTuple_SetItem(tup, i, PyUnicode_FromString(str)) != 0) {
			PyErr_BadInternalCall();
			return NULL;
		}
		free(str);
	}

	Py_XDECREF(seq);
	return tup;
}

/*	Initializing UniqueList. */
static int
UniqueList_init(UniqueList *self, PyObject *args, PyObject *kwds) {
	Py_ssize_t i;
	PyObject *tmp;
	PyObject *item, *obj = NULL;

	if (!PyArg_ParseTuple(args, "|O", &obj))
	        return -1;

	/* Checking, that the argument is containing unique values. */
	if (obj != NULL) {
	    tmp = get_lowercase_tuple(obj);
		for (i = 0; i < Py_SIZE(tmp); i++) {
			item = PyTuple_GetItem(tmp, i);
			if (PySequence_Count(tmp, item) > 1) {
				Py_DECREF(tmp);
				PyErr_SetString(PyExc_AttributeError, "LDAPListValue's argument is containing non-unique values. (Bool types converted to number)");
				return -1;
			}
		}
		Py_DECREF(tmp);
	}
	if (PyList_Type.tp_init((PyObject *)self, args, kwds) < 0)
		return -1;
	return 0;
}

/*	Create a new UniqueList object for internal use. */
UniqueList *
UniqueList_New(void) {
	UniqueList *self = (UniqueList *)UniqueListType.tp_new(&UniqueListType, NULL, NULL);
	return self;
}

/*	Returns 1 if the `newitem` is not in the list, 0 otherwise.
	The function uses the lowerCaseMatch() to compare the list's items and the `newitem`
	to lower-case C string.
*/
static int
isLowerCaseUnique(UniqueList *list, PyObject *newitem) {
	Py_ssize_t i;

	for (i = 0; i < Py_SIZE(list); i++) {
		if (lowerCaseMatch(list->list.ob_item[i], newitem) == 1) {
			return 0;
		}
	}
	return 1;
}

/*	Append new - case-insensitive - unique item to the UniqueList. */
int
UniqueList_Append(UniqueList *self, PyObject *newitem) {
	if (isLowerCaseUnique(self, newitem) == 0) {
		PyErr_Format(PyExc_ValueError, "%R is already in the list.", newitem);
		return -1;
	}

	return PyList_Append((PyObject *)self, newitem);
}

/*	Returns 1 if obj is an instance of UniqueList, or 0 if not.
	On error, returns -1 and sets an exception.
*/
int
UniqueList_Check(PyObject *obj) {
	if (obj == NULL) return -1;
	return PyObject_IsInstance(obj, (PyObject *)&UniqueListType);
}

int
UniqueList_Extend(UniqueList *self, PyObject *b) {
	PyObject *iter = NULL, *newitem, *ret;

	if (b != NULL) iter = PyObject_GetIter(b);

	if (iter != NULL) {
		for (newitem = PyIter_Next(iter); newitem != NULL; newitem = PyIter_Next(iter)) {
			if (isLowerCaseUnique(self, newitem) == 0){
				PyErr_SetString(PyExc_TypeError, "List is containing non-unique values.");
				return -1;
			}
			Py_DECREF(newitem);
		}
		Py_DECREF(iter);
	}

	ret = _PyList_Extend((PyListObject *)self, b);
	if (ret != Py_None) return -1;
	return 0;
}

/*	Insert new unique item to the `where` position in UniqueList. */
int
UniqueList_Insert(UniqueList *self, Py_ssize_t where, PyObject *newitem) {
	if (isLowerCaseUnique(self, newitem) == 0) {
		PyErr_Format(PyExc_ValueError, "%R is already in the list.", newitem);
		return -1;
	}

    return PyList_Insert((PyObject *)self, where, newitem);
}

int
UniqueList_Remove_wFlg(UniqueList *self, PyObject *value) {
	int cmp;
	Py_ssize_t i;

	for (i = 0; i < Py_SIZE(self); i++) {
		cmp = lowerCaseMatch(self->list.ob_item[i], value);
		if (cmp > 0) {
			if (UniqueList_SetSlice(self, i, i+1, (PyObject *)NULL) == 0) {
				return 1;
			}
			return -1;
		} else if (cmp < 0) return -1;
	}
	return 0;
}

int
UniqueList_Remove(UniqueList *self, PyObject *value) {
	int rc;

	rc = UniqueList_Remove_wFlg(self, value);
	if (rc == 0) {
		PyErr_SetString(PyExc_ValueError, "LDAPListValue.remove(x): x not in list");
		return -1;
	} else if (rc == 1) {
		return 0;
	}
	return -1;
}

/*	Set new unique item at `i` index in UniqueList to `newitem`. Case-insensitive. */
int
UniqueList_SetItem(UniqueList *self, Py_ssize_t i, PyObject *newitem) {
	if (isLowerCaseUnique(self, newitem) == 0) {
		PyErr_Format(PyExc_ValueError, "%R is already in the list.", newitem);
		return -1;
	}
	return PyList_SetItem((PyObject *)self, i, newitem);
}

/*	Set the slice of UniqueList between `ilow` and `ihigh` to the contents of `itemlist`.
	The `itemlist` must be containing unique elements. The `itemlist` may be NULL, indicating
	the assignment of an empty list (slice deletion).
*/
int
UniqueList_SetSlice(UniqueList *self, Py_ssize_t ilow, Py_ssize_t ihigh, PyObject *itemlist) {
	PyObject *iter = NULL;
	PyObject *newitem;

	if (itemlist != NULL) iter = PyObject_GetIter(itemlist);

	if (iter != NULL) {
		for (newitem = PyIter_Next(iter); newitem != NULL; newitem = PyIter_Next(iter)) {
			if (isLowerCaseUnique(self, newitem) == 0) {
				PyErr_Format(PyExc_ValueError, "%R is already in the list.", newitem);
				return -1;
			}
			Py_DECREF(newitem);
		}
		Py_DECREF(iter);
	}

    return PyList_SetSlice((PyObject *)self, ilow, ihigh, itemlist);
}

static PyObject *
UL_append(UniqueList *self, PyObject *newitem) {
    if (UniqueList_Append(self, newitem) == 0) {
    	return Py_None;
    }
    return NULL;
}

static PyObject *
UL_extend(UniqueList *self, PyObject *b) {
	if (UniqueList_Extend(self, b) == 0) {
		return Py_None;
	}
	return NULL;
}

static PyObject *
UL_insert(UniqueList *self, PyObject *args) {
    Py_ssize_t i;
    PyObject *v;

    if (!PyArg_ParseTuple(args, "nO:insert", &i, &v)) return NULL;
    if (UniqueList_Insert(self, i, v) == 0) {
    	return Py_None;
    }
    return NULL;
}

static PyObject *
UL_remove(UniqueList *self, PyObject *value) {
	if (UniqueList_Remove(self, value) == 0) {
		return Py_None;
	}
	return NULL;
}

static PyMethodDef UniqueList_methods[] = {
    {"append", 	(PyCFunction)UL_append, 	METH_O, "Append new item to the UniqueList." },
    {"extend",  (PyCFunction)UL_extend,  	METH_O, "Extend UniqueList."},
    {"insert", 	(PyCFunction)UL_insert,	METH_VARARGS, "Insert new item in the UniqueList."},
    {"remove", 	(PyCFunction)UL_remove, 	METH_O, "Remove item from UniqueList."},
    {NULL, NULL, 0, NULL}  /* Sentinel */
};

static PyObject *
UL_concat(UniqueList *self, PyObject *bb) {
	UniqueList *np = UniqueList_New();

	if (np == NULL) return PyErr_NoMemory();

	if (UL_extend(np, (PyObject *)self) == NULL) return NULL;
	if (UL_extend(np, bb) == NULL) return NULL;

	return (PyObject *)np;
}

static int
UL_ass_item(UniqueList *self, Py_ssize_t i, PyObject *v) {
    if (i < 0 || i >= Py_SIZE(self)) {
        PyErr_SetString(PyExc_IndexError,
                        "list assignment index out of range");
        return -1;
    }
    if (v == NULL) return UniqueList_SetSlice(self, i, i+1, v);
    return UniqueList_SetItem(self, i, v);
}

static int
UL_contains(UniqueList *self, PyObject *el) {
    int cmp;
	Py_ssize_t i;
	PyObject *tup;

	tup = get_lowercase_tuple((PyObject *)self);
	if (tup == NULL) return -1;

    for (i = 0, cmp = 0 ; cmp == 0 && i < Py_SIZE(tup); ++i) {
    	cmp = lowerCaseMatch(PyTuple_GetItem(tup, i), el);
    }
    return cmp;
}

static PyObject *
UL_inplace_concat(UniqueList *self, PyObject *other) {
    PyObject *result;

    result = UL_extend(self, other);
    if (result == NULL)
        return result;
    Py_DECREF(result);
    Py_INCREF(self);
    return (PyObject *)self;
}

static PyObject *
UL_dummy(UniqueList *self, Py_ssize_t n) {
	PyErr_SetString(PyExc_TypeError, "unsupported operand.");
	return NULL;
}

static PySequenceMethods UL_as_sequence = {
    0,                      		/* sq_length */
    (binaryfunc)UL_concat,			/* sq_concat */
    (ssizeargfunc)UL_dummy,        	/* sq_repeat */
    0,                    			/* sq_item */
    0,                             	/* sq_slice */
    (ssizeobjargproc)UL_ass_item,	/* sq_ass_item */
    0,                              /* sq_ass_slice */
    (objobjproc)UL_contains,       	/* sq_contains */
    (binaryfunc)UL_inplace_concat,	/* sq_inplace_concat */
    (ssizeargfunc)UL_dummy,        	/* sq_inplace_repeat */
};

/*	This function is based on list_ass_subscript from Python source listobject.c.
	But it uses UniqueList's setslice and setitem functions instead of memory opertation.
	It is probably a much more slower, but cleaner (easier) solution.
*/
static int
UL_ass_subscript(UniqueList *self, PyObject *item, PyObject *value) {
    size_t cur;
	Py_ssize_t i;
	Py_ssize_t start, stop, step, slicelength;
    PyObject *seq;
    PyObject **seqitems;

	if (PyIndex_Check(item)) {
		i = PyNumber_AsSsize_t(item, PyExc_IndexError);
		if (i == -1 && PyErr_Occurred()) return -1;
		if (i < 0) i += PyList_GET_SIZE(self);
		return UL_ass_item(self, i, value);
    } else if (PySlice_Check(item)) {
    	if (PySlice_GetIndicesEx(item, Py_SIZE(self), &start, &stop, &step, &slicelength) < 0) {
    		return -1;
    	}

    	if (step == 1) return UniqueList_SetSlice(self, start, stop, value);

        /* Make sure s[5:2] = [..] inserts at the right place:
           before 5, not before 2. */
    	if ((step < 0 && start < stop) || (step > 0 && start > stop)) {
    		stop = start;
    	}

    	if (value == NULL) {
    		/* delete slice */
            if (slicelength <= 0) return 0;

            if (step < 0) {
                stop = start + 1;
                start = stop + step*(slicelength - 1) - 1;
                step = -step;
            }

            for (cur = start, i = 0; cur < (size_t)stop; cur += step, i++) {
            	if (UniqueList_SetSlice(self, cur-i, cur+1-i, (PyObject *)NULL) != 0) {
            		return -1;
            	}
            }
            return 0;
        } else {
        	/* assign slice */
        	/* protect against a[::-1] = a */
        	if (self == (UniqueList *)value) {
        		seq = PyList_GetSlice(value, 0, PyList_GET_SIZE(value));
        	} else {
        		seq = PySequence_Fast(value, "must assign iterable to extended slice");
        	}
        	if (!seq) return -1;

        	if (PySequence_Fast_GET_SIZE(seq) != slicelength) {
        		PyErr_Format(PyExc_ValueError, "attempt to assign sequence of size %zd to extended slice of "
        				"size %zd", PySequence_Fast_GET_SIZE(seq), slicelength);
        		Py_DECREF(seq);
        		return -1;
        	}

        	if (!slicelength) {
        		Py_DECREF(seq);
        		return 0;
        	}

        	seqitems = PySequence_Fast_ITEMS(seq);
        	for (cur = start, i = 0; i < slicelength; cur += (size_t)step, i++) {
        		if (UniqueList_SetItem(self, cur, seqitems[i]) != 0) {
        			return -1;
        		}
        	}
        	Py_DECREF(seq);
        	return 0;
        }
    } else {
    	PyErr_Format(PyExc_TypeError, "list indices must be integers, not %.200s", item->ob_type->tp_name);
    	return -1;
    }
}

static PyMappingMethods UL_as_mapping = {
    0,									/* mp_length */
    0,									/* mp_subscript */
    (objobjargproc)UL_ass_subscript,	/* mp_ass_subscript */
};

PyTypeObject UniqueListType = {
    PyObject_HEAD_INIT(NULL)
    "pyLDAP.UniqueList",      /* tp_name */
    sizeof(UniqueList),       /* tp_basicsize */
    0,                       /* tp_itemsize */
    (destructor)UniqueList_dealloc,/* tp_dealloc */
    0,                       /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_reserved */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    &UL_as_sequence,        /* tp_as_sequence */
    &UL_as_mapping,		 /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    0,                       /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                       /* tp_doc */
    0,    					 /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    UniqueList_methods,   /* tp_methods */
    0,       				 /* tp_members */
    0,    					 /* tp_getset */
    &PyList_Type,            /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)UniqueList_init,/* tp_init */
    0,                       /* tp_alloc */
    UniqueList_new,      /* tp_new */
};
