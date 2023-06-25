#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define BUFFER_SIZE 1024
#define MAX_RATE 4
#define MEMORY_ALLOCATION_ERROR  (-1)

typedef union {
	int int_data;
	char char_data;
	// ... other primitive types used
	void *ptr_data;
} data_union;

typedef struct ht_element {
	struct ht_element *next;
	data_union data;
} ht_element;

typedef void (*DataFp)(data_union);
typedef void (*DataPFp)(data_union*);
typedef int (*CompareDataFp)(data_union, data_union);
typedef size_t (*HashFp)(data_union, size_t);
typedef data_union (*CreateDataFp)(void*);

typedef struct {
	size_t size;
	size_t no_elements;
	ht_element **ht;
	DataFp dump_data;
	CreateDataFp create_data;
	DataFp free_data;
	CompareDataFp compare_data;
	HashFp hash_function;
	DataPFp modify_data;
} hash_table;

// ---------------------- functions to implement

// initialize table fields
void init_ht(hash_table *p_table, size_t size, DataFp dump_data, CreateDataFp create_data,
		 DataFp free_data, CompareDataFp compare_data, HashFp hash_function, DataPFp modify_data) {
	p_table->size = size;
	p_table->no_elements = 0;
	p_table->ht = malloc(size * sizeof(ht_element *));
	if (p_table->ht == NULL) 
		return;
	for (size_t i = 0; i < size; i++) *(p_table->ht+i) = NULL;
	p_table->dump_data = dump_data;
	p_table->create_data = create_data;
	p_table->free_data = free_data;
	p_table->compare_data = compare_data;
	p_table->hash_function = hash_function;
	p_table->modify_data = modify_data;
}

// print elements of the list with hash n
void dump_list(const hash_table* p_table, size_t n) {
	ht_element *elem = *(p_table->ht+n);
	while (elem != NULL)
	{
		p_table->dump_data(elem->data);
		elem = elem->next;
	}
}

// Free element pointed by data_union using free_data() function
void free_element(DataFp free_data, ht_element *to_delete) {
	free_data(to_delete->data);
}

// free all elements from the table (and the table itself)
void free_table(hash_table* p_table) {
	for (size_t i = 0; i < p_table->size; i++)
	{
		ht_element *elem = *(p_table->ht+i);
		while (elem != NULL)
		{
			free_element(p_table->free_data, elem);
			elem = elem->next;
		}
	}
	free(p_table);
}

// calculate hash function for integer k
size_t hash_base(int k, size_t size) {
	static const double c = 0.618033988; // (sqrt(5.) â€“ 1) / 2.;
	double tmp = k * c;
	return (size_t)floor((double)size * (tmp - floor(tmp)));
}

void add_to_ht_list(ht_element **list, ht_element *new_elem, CompareDataFp cmp_fn) {
	ht_element *elem = *list;
	if (elem == NULL)
	{
		new_elem->next = NULL;
		*list = new_elem; 
	}

	while (elem != NULL)
	{
		if (cmp_fn(elem->data, new_elem->data) > 0 || elem->next == NULL)
		{
			new_elem->next = elem->next;
			elem->next = new_elem;
			return;
		}
		elem = elem->next;
	}
}

void rehash(hash_table *p_table) {
	size_t new_size = p_table->size * 2;
	ht_element **lists = malloc(sizeof(ht_element *) * new_size);
	if (lists == NULL)
		return;
	for (size_t i = 0; i < p_table->size; i++)
	{
		ht_element *elem = *(p_table->ht+i);
		while (elem != NULL)
		{
			// memorize the next element of this list, elem will change
			// it's next after addition to new list
			ht_element *next_elem = elem->next;
			size_t nh = p_table->hash_function(elem->data, new_size);
			add_to_ht_list(lists+nh, elem, p_table->compare_data);
			elem = next_elem;
		}
	}
	p_table->size = new_size;
	// only free previous lists pointers
	free(p_table->ht);
	p_table->ht = lists;
}

// find element; return pointer to previous
ht_element *find_previous(hash_table *p_table, data_union data) {
	// ?
}

// return pointer to element with given value
ht_element *get_element(hash_table *p_table, data_union *data) {
	size_t h = p_table->hash_function(*data, p_table->size);
	ht_element *elem = *(p_table->ht+h);
	while (elem != NULL)
	{
		if (p_table->compare_data(elem->data, *data) == 0)
			return elem;
		elem = elem->next;
	}
	return NULL;
}



// insert element
void insert_element(hash_table *p_table, data_union *data) {
	size_t h = p_table->hash_function(*data, p_table->size);
	ht_element *new_elem = malloc(sizeof(ht_element *));
	if (new_elem == NULL)
		return;
	new_elem->data = *data;

	add_to_ht_list(p_table->ht+h, new_elem, p_table->compare_data);
}

// remove element
void remove_element(hash_table *p_table, data_union data) {
	size_t h = p_table->hash_function(data, p_table->size);
	ht_element *prev_elem = *(p_table->ht+h);
	ht_element *elem = prev_elem->next;

	if (p_table->compare_data(prev_elem->data, data) == 0)
	{
		*(p_table->ht+h) = elem;
		free_element(p_table->free_data, prev_elem);
		return;
	}

	while (elem != NULL)
	{
		if (p_table->compare_data(elem->data, data) == 0)
		{
			prev_elem->next = elem->next;
			free_element(p_table->free_data, elem);
			return;
		}
		prev_elem = elem;
		elem = elem->next;
	}
}

// type-specific definitions

// int element

size_t hash_int(data_union data, size_t size) {
	return hash_base(data.int_data, size);
}

void dump_int(data_union data) {
	printf("%d\n", data.int_data);
}

int cmp_int(data_union a, data_union b) {
	return a.int_data - b.int_data;
}

data_union create_int(void* value) {
	data_union du;
	if (value == NULL)
		scanf("%d", &du.int_data);
	else
		du.int_data = *((int *)value);
	return du;
}

// char element

size_t hash_char(data_union data, size_t size) {
	return hash_base((int)data.char_data, size);
}

void dump_char(data_union data) {
	putchar(data.char_data);
}

int cmp_char(data_union a, data_union b) {
	return a.char_data - b.char_data;
}

data_union create_char(void* value) {
	data_union du;
	if (value == NULL)
		scanf("%c", &du.char_data);
	else
		du.char_data = *((char *)value);
	return du;
}

// Word element

typedef struct DataWord {
	char *word;
	int counter;
} DataWord;

void dump_word(data_union data) {
	DataWord *dw = data.ptr_data;
	if (dw == NULL)
		return;
	printf("%s %d\n", dw->word, dw->counter);
}

void free_word(data_union data) {
	DataWord *dw = data.ptr_data;
	if (dw == NULL)
		return;
	free(dw->word);
	free(dw);
}

int cmp_word(data_union a, data_union b) {
	DataWord *dwa = a.ptr_data;
	DataWord *dwb = b.ptr_data;
	if (dwa == NULL && dwb == NULL) return 0;
	if (dwa == NULL) return -1;
	if (dwb == NULL) return 1;

	return strcmp(dwa->word, dwb->word);
}

size_t hash_word(data_union data, size_t size) {
	int s = 0;
	DataWord *dw = (DataWord*)data.ptr_data;
	char *p = dw->word;
	while (*p) {
		s += *p++;
	}
	return hash_base(s, size);
}

void modify_word(data_union *data) {
	DataWord *dw = data->ptr_data;
	if (dw == NULL)
		return;
	dw->counter++;
}

data_union create_data_word(void *value) {
	data_union du;
	DataWord *dw = malloc(sizeof(DataWord));
	du.ptr_data = dw;
	if (dw == NULL) return du;
	dw->word = value;
	dw->counter = 1;
	return du;
}

// read text, parse it to words, and insert these words to the hashtable
void stream_to_ht(hash_table *p_table, FILE *stream) {
	int c;
	int is_break = 1;

	char *buf = calloc(BUFFER_SIZE, sizeof(char));
	if (buf == NULL)
		return;
	char *bufptr = buf;

	for (;;)
	{
		c = getchar();
		int is_wspace = c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == EOF;
		if (is_wspace && is_break) 
			continue;

		if (is_wspace && !is_break)
		{
			is_break = 1;
			*bufptr = '\0';
			size_t buflen = strnlen(buf, BUFFER_SIZE) + 1;
			char *wbuf = malloc(buflen);
			if (wbuf == NULL)
				return;
			memcpy(wbuf, buf, buflen);
			// add to the table
			data_union dw = create_data_word(wbuf);	
			insert_element(p_table, &dw);
			bufptr = buf;
		}
		else
		{
			is_break = 0;
			*bufptr = c;
			bufptr++;
		}

		if (c == EOF)
			break;
	}
}

// test primitive type list
void test_ht(hash_table *p_table, int n) {
	char op;
	data_union data;
	for (int i = 0; i < n; ++i) {
		scanf(" %c", &op);
//		p_table->create_data(&data);
		data = p_table->create_data(NULL); // should give the same result as the line above
		switch (op) {
			case 'r':
				remove_element(p_table, data);
				break;
			case 'i':
				insert_element(p_table, &data);
				break;
			default:
				printf("No such operation: %c\n", op);
				break;
		}
	}
}

int main(void) {
	int to_do, n;
	size_t index;
	hash_table table;
	char buffer[BUFFER_SIZE];
	data_union data;

	scanf ("%d", &to_do);
	switch (to_do) {
		case 1: // test integer hash table
			scanf("%d %zu", &n, &index);
			init_ht(&table, 4, dump_int, create_int, NULL, cmp_int, hash_int, NULL);
			test_ht(&table, n);
			printf ("%zu\n", table.size);
			dump_list(&table, index);
			break;
		case 2: // test char hash table
			scanf("%d %zu", &n, &index);
			init_ht(&table, 4, dump_char, create_char, NULL, cmp_char, hash_char, NULL);
			test_ht(&table, n);
			printf ("%zu\n", table.size);
			dump_list(&table, index);
			break;
		case 3: // read words from text, insert into hash table, and print
			scanf("%s", buffer);
			init_ht(&table, 8, dump_word, create_data_word, free_word, cmp_word, hash_word, modify_word);
			stream_to_ht(&table, stdin);
			printf ("%zu\n", table.size);
			data = table.create_data(buffer);
			ht_element *e = get_element(&table, &data);
			if (e) table.dump_data(e->data);
			if (table.free_data) table.free_data(data);
			break;
		default:
			printf("NOTHING TO DO FOR %d\n", to_do);
			break;
	}
	free_table(&table);

	return 0;
}
