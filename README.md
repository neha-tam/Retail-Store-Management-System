# Retail-Store-Management-System

This is an implementation of a retail store management system using C language. This project includes features such as file management (file locking) and socket programming.

There are 4 main files of code, which are Server.c, User.c, Admin.c and Server.h. The Admin
and User connect to the server using the same port number, as written in the code.
The Inventory and the carts of the users are stored in files called Inventory.dat and Cart.dat, which are initially created when the program is first run. In the Server.h file, structures which are used throughout the code are defined. Each Inventory item is stored as an InventoryItem structure, and each record in the Inventory file will be this structure. The information about the Inventory file is stored in the InventoryInfo structure, which contains the file descriptor for the Inventory file. Another file called Keepcount.dat keeps count of the quantities of the products in the Inventory. This file is also created when the code is first run. Carts are stored as a structure called Cart, and the records in the Cart file are these structures. Each cart has a customer id and is identified by that. A Cart structure has an array of products that it contains, and each element in the array is a structure called CartItem, which contains details of the product. A structure CartInfo exists, which contains the details of the Carts, such as the file descriptor of the Cart file. 

Only the Server file accesses files in the system, the User.c and Admin.c files act like interfaces for the respective entities.

**To run the code:**
Complie each of the 3 .c files, and run each of them on separate terminals.

_Assumptions_: A product with a specific product name can have only 1 price.
