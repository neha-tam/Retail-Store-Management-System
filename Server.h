#ifndef STORE_OPERATIONS_H
#define STORE_OPERATIONS_H

// Error codes
#define SO_SUCCESS 0
#define SO_FILE_ERROR 1

//Inventory error codes
#define INVENTORY_ADD_FAILED 2
#define INVENTORY_ITEM_NOT_FOUND 3
#define INVENTORY_ALREADY_OPEN 4
#define INVENTORY_NOT_OPEN 5
#define INVENTORY_DELETE_FAILED 6

//Cart error codes
#define CART_ADD_FAILED 7
#define CART_ITEM_NOT_FOUND 8
#define CART_ALREADY_OPEN 9
#define CART_NOT_OPEN 10


#define INV_NDX_SAVE_FAILED 11
#define CART_NDX_SAVE_FAILED 12


// Status values
#define INVENTORY_OPEN 13
#define INVENTORY_CLOSED 14
#define CART_OPEN 15
#define CART_CLOSED 16

#define BUY_FAILED 17

#define MAX_INVENTORY_SIZE 5
#define MAX_FREE 100	// Maximum of deleted entries info
#define MAX_CARTS  5   // Maximum number items in a cart
#define MAX_CART_SIZE 10    // Maximum number items in a cart

struct InventoryItem
{
	int productID;
	char productName[100];
	int price;
    int valid;
};


struct InventoryInfo
{
    int inv_data_fd;
	int inv_quantity_fd;
	int inv_status; 
	int inv_entry_size; // size of one inventory entry
	int inv_count; // For the number of items in the inventory, to be incremented each time an item is added by the admin
};

struct InventoryCount
{
	char prodName[100];
	int quantity;
};



struct CartItem
{
	char prodName[100];
	int price;
	int quantity;
};

struct Cart
{
	int custID;
	struct CartItem cartItems[MAX_CART_SIZE];
	int noOfProducts;
	int valid;
};

struct CartInfo
{
	int cart_data_fd;
	int cart_ndx_fd;
	int cart_status;
	int cart_entry_size;  //size of one cart entry
	int cart_count; //number of entries in the cart file

};

extern struct InventoryInfo inv_handle;
extern struct CartInfo cart_handle;

int cart_create();
int cart_open();
int cart_load_ndx();
int cart_put_rec( int id, void *rec );
int cart_get_rec(int id, struct Cart *cart);
int cart_update(int id, void *newrec);
int cart_add(int custID,char pname[],int price, int quan);
int cart_delete(int transID,int productID);
int cart_display(int custID, int nsd);
int cart_edit(int custID, char pname[], int quan);
int cart_delete_item(int custID, char pname[]);
int cart_close();


int inv_create();
int inv_open();
int inv_count_load();
int inv_add(int prodID,char* prodName,int price);
int inv_delete(int prodID);
int inv_update(int prodID,char* prodName, int price);
int inv_display();
int inv_close();

int log_create();
int log_open();



#endif