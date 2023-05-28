#include<unistd.h>
#include<stdio.h>
#include<string.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include "Server.h"

#define PORTNO 8004

struct InventoryInfo inv_handle;
struct CartInfo cart_handle;

int log_fd=-1; //file descriptor for the log
struct flock lockl;

int log_create()
{
    int log_fd = open("Log.txt",O_RDONLY | O_CREAT, 0644);
    
    if(log_fd == -1)
    {
        perror("Inventory/Keepcount file could not be created\n");
        return SO_FILE_ERROR;
    }

    close(log_fd);

}

int log_open()
{
    int log_fd = open("Log.txt",O_RDWR, 0644); 
    if(log_fd == -1)
    {
        perror("Log cannot be opened\n");
        return SO_FILE_ERROR;
    }
}

int inv_create()
{
    int inv_fd = open("Inventory.dat",O_RDONLY | O_CREAT, 0644);
    int count_fd = open("Keepcount.dat",O_RDONLY | O_CREAT, 0644);  //this file is used to keep a count of the quantities in the inventory
    if(inv_fd == -1 || count_fd == -1)
    {
        perror("Inventory/Keepcount file could not be created\n");
        return SO_FILE_ERROR;
    }

    close(inv_fd);
    close(count_fd);
}

int inv_open()
{
    int inv_fd = open("Inventory.dat",O_RDWR, 0644);
    int count_fd = open("Keepcount.dat",O_RDWR, 0644);

    if(inv_fd == -1 || count_fd == -1)
    {
        perror("Inventory/Keepcount file could not be opened\n");
        return SO_FILE_ERROR;
    }

    if (inv_handle.inv_status  == INVENTORY_OPEN)
    {
        return INVENTORY_ALREADY_OPEN;
    }
    else
    {
    	inv_handle.inv_entry_size = sizeof(struct InventoryItem);
    	inv_handle.inv_data_fd = inv_fd;
        inv_handle.inv_quantity_fd = count_fd;
        inv_handle.inv_status = INVENTORY_OPEN;

		return inv_count_load();
	}
}

int inCount(char prodName[],struct InventoryCount inv[])
{
    for(int i=0;i<MAX_INVENTORY_SIZE;i++)
    {
        if(!strcmp(inv[i].prodName,prodName))
        {
            return i;
        }
    }

    return -1;
}

int inv_count_load()
{
    struct InventoryItem item;
    struct InventoryCount count[MAX_INVENTORY_SIZE];

    //initialization
    for(int i=0;i<MAX_INVENTORY_SIZE;i++)
    {
        strcpy(count[i].prodName,"");
        count[i].quantity = 0;
    }

    int i = 0;
    int isInCount = 0;

    struct flock lock;  //setting a read lock on the whole file to check whether that product ID already has been added
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=lseek(inv_handle.inv_data_fd,0,SEEK_END);
    fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
    // printf("Set the read lock.\n");
    
    lseek(inv_handle.inv_data_fd,0,SEEK_SET);
    while(read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem)))
    {
        if(item.valid == 1)
        {
            isInCount = inCount(item.productName,count);
            if(isInCount != -1)
            {
                count[isInCount].quantity++;
            }
            else
            {
                for(int j = 0;j<MAX_INVENTORY_SIZE;j++)
                {
                    if(!strcmp(count[j].prodName,""))
                    {
                        //this is to add a new product to the Keepcount file (first we store the items in an array)
                        strcpy(count[j].prodName,item.productName);
                        count[j].quantity = 1;
                        break;
                    }
                }
            }  
            i++;
            if(i == MAX_INVENTORY_SIZE)    ///gone through maximum number of inventory items allowed
            {
                break;
            } 

        }
            
    }
    lock.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
    // printf("Released the read lock.\n");

    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=lseek(inv_handle.inv_quantity_fd,0,SEEK_END);
    fcntl(inv_handle.inv_quantity_fd,F_SETLKW,&lock);
    // printf("Write locked in inv_add()\n");
    lseek(inv_handle.inv_quantity_fd,0,SEEK_SET);
    for(int i=0;i<MAX_INVENTORY_SIZE;i++)
    {
        write(inv_handle.inv_quantity_fd,&count[i],sizeof(struct InventoryCount));
    }

    lock.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lock);  //unlock
    

    return SO_SUCCESS;
    
}

int inv_add(int prodID,char* prodName,int price)
{
    if( inv_handle.inv_status == INVENTORY_CLOSED)
	{
        return SO_FILE_ERROR;
	}
    else
    {
        struct InventoryItem inv;
        int offset = lseek(inv_handle.inv_data_fd,0,SEEK_END);
        int look_for_offset = 1;
        int go_back_offset = -1;

        

        
        
        struct flock lock;  //setting a read lock on the whole file to check whether that product ID already has been added
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=0;
        lock.l_len=lseek(inv_handle.inv_data_fd,0,SEEK_END);
        fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
        // printf("Setting the read lock.\n");
        

        lseek(inv_handle.inv_data_fd,0,SEEK_SET);
        while(read(inv_handle.inv_data_fd,&inv,sizeof(struct InventoryItem)))
        {
            if(inv.productID == prodID && inv.valid == 1)   //a valid product with that product ID has already been added
            {
                lock.l_type=F_UNLCK;    
                fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
                return INVENTORY_ADD_FAILED;
            }
            else if(inv.valid == 0 && look_for_offset == 1)
            {
                go_back_offset = sizeof(struct InventoryItem);
                offset = lseek(inv_handle.inv_data_fd,-go_back_offset,SEEK_CUR);
                look_for_offset = 0;
            }
        }
        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
        // printf("Released the read lock.\n");

        printf("Checked if this is a valid prodID \n");
        
        //inventory file

        if(offset == lseek(inv_handle.inv_data_fd,0,SEEK_END))  
        {    
            ftruncate(inv_handle.inv_data_fd,offset+sizeof(struct InventoryItem));
        }
        
        printf("ftruncated or whatever\n");
        inv.productID = prodID;
        strcpy(inv.productName,prodName);
        inv.price = price;
        inv.valid = 1;
        
        //record locking
        lock.l_type=F_WRLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct InventoryItem);
        fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
        

        lseek(inv_handle.inv_data_fd,offset,SEEK_SET);
        int bytes = write(inv_handle.inv_data_fd,&inv,inv_handle.inv_entry_size);

        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);  //unlock
        // printf("Unlocked the write lock.\n");
        
        struct InventoryCount count_buf,count;
        int num = 0;    //temporary number
        strcpy(count.prodName,prodName);
        
        lock.l_type=F_WRLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=0;
        lock.l_len=lseek(inv_handle.inv_quantity_fd,0,SEEK_END);
        fcntl(inv_handle.inv_quantity_fd,F_SETLKW,&lock);


        printf("Write locked in add product\n");
        lseek(inv_handle.inv_quantity_fd,0,SEEK_SET);
        while(read(inv_handle.inv_quantity_fd,&count_buf,sizeof(struct InventoryCount)))
        {
            if(!strcmp(count_buf.prodName,prodName))
            {
                lseek(inv_handle.inv_quantity_fd,-sizeof(struct InventoryCount),SEEK_CUR);
                num = count_buf.quantity +1;
                count.quantity = num;
                write(inv_handle.inv_quantity_fd,&count,sizeof(struct InventoryCount));
                break;
            }
            else if(!strcmp(count_buf.prodName,""))   //find the first null entry, cuz a new product is added
            {
                go_back_offset = sizeof(struct InventoryCount);
                lseek(inv_handle.inv_quantity_fd,-go_back_offset,SEEK_CUR);
                count.quantity = 1;
                
                write(inv_handle.inv_quantity_fd,&count,sizeof(struct InventoryCount));
                break;
            }
        }

        printf("GOung to print in add produst to see if it's done\n");
        //unlock
        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lock);  
        printf("Unlocked the quantity file\n");


        log_fd = log_open();
        lockl.l_type=F_WRLCK;
        lockl.l_whence=SEEK_SET;
        lockl.l_start=0;
        lockl.l_len=lseek(log_fd,0,SEEK_END);
        fcntl(log_fd,F_SETLKW,&lockl);

        

        
        lseek(log_fd, 0, SEEK_END);
        write(log_fd, "Product added to the inventory: ", sizeof("Product added to the inventory: "));
        int co=0;
        while(prodName[co] !='\0')
        {
            co++;
        }
       
        write(log_fd, prodName, co);
        write(log_fd, "\n\r", sizeof("\n"));

        lockl.l_type=F_UNLCK;    
        fcntl(log_fd,F_SETLK,&lockl);  


        return SO_SUCCESS;
    }
}




int inv_update(int prodID,char* prodName, int price)
{
    struct InventoryItem item, old_item;
    if( inv_handle.inv_status == INVENTORY_CLOSED)
	{
		return SO_FILE_ERROR;
	}
    else
    {
        struct InventoryItem item, old_item;
        int deleted = 1;
        int offset = lseek(inv_handle.inv_data_fd,0,SEEK_END);
        int go_back_offset=-1;

        struct flock lock;  //setting a read lock on the whole file to check whether that product ID already has been added
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=0;
        lock.l_len=offset;
        fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
        // printf("Setting the read lock.\n");
        lseek(inv_handle.inv_data_fd,0,SEEK_SET); 

        while(read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem)))
        {
            if(item.valid == 1)
            {
                if(item.productID == prodID)
                {  
                    deleted = 0;
                    go_back_offset = sizeof(struct InventoryItem);
                    offset = lseek(inv_handle.inv_data_fd,-go_back_offset,SEEK_CUR);
                    lseek(inv_handle.inv_data_fd,offset,SEEK_SET);
                    read(inv_handle.inv_data_fd,&old_item,sizeof(old_item));    //to read the old item which is to be updated
                    break;
                }
            }
            
        }

        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
        // printf("Released the read lock.\n");

        if(deleted == 1)
        {
            return INVENTORY_ITEM_NOT_FOUND;
        }
        item.productID = prodID;
        strcpy(item.productName,prodName);
        item.price = price;
        item.valid = 1;

        lock.l_type=F_WRLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=offset;
        lock.l_len=sizeof(struct InventoryItem);
        fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
        // printf("Locked for writing.\n");
        lseek(inv_handle.inv_data_fd,offset,SEEK_SET);
        write(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem));
        
        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
        // printf("Released the write lock.\n");

        struct InventoryCount count_buf,count;
        int both_jobs = 0;
        strcpy(count.prodName,prodName);
        
        lock.l_type=F_WRLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=0;
        lock.l_len=lseek(inv_handle.inv_quantity_fd,0,SEEK_END);
        fcntl(inv_handle.inv_quantity_fd,F_SETLKW,&lock);

        if(strcmp(old_item.productName,prodName))
        {
            while(read(inv_handle.inv_quantity_fd,&count_buf,sizeof(struct InventoryCount)))
            {
                if(!strcmp(count_buf.prodName,prodName))   
                {
                    lseek(inv_handle.inv_quantity_fd,-sizeof(struct InventoryCount),SEEK_CUR);
                    count.quantity = count_buf.quantity + 1;
                    write(inv_handle.inv_quantity_fd,&count,sizeof(struct InventoryCount));
                    both_jobs++;
                }
                if(!strcmp(count_buf.prodName,old_item.productName))
                {
                    lseek(inv_handle.inv_quantity_fd,-sizeof(struct InventoryCount),SEEK_CUR);
                    count.quantity = count_buf.quantity - 1;
                    write(inv_handle.inv_quantity_fd,&count,sizeof(struct InventoryCount));
                    both_jobs++;
                }

                if(both_jobs == 2)
                {
                    break;
                }
            }
        }
        //unlock
        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lock);  
        
    }

        log_fd = log_open();
        lockl.l_type=F_WRLCK;
        lockl.l_whence=SEEK_SET;
        lockl.l_start=0;
        lockl.l_len=lseek(log_fd,0,SEEK_END);
        fcntl(log_fd,F_SETLKW,&lockl);

        

        printf("Locked the log file\n");
        lseek(log_fd, 0, SEEK_END);
        write(log_fd, "Product updated in the inventory: ", sizeof("Product updated in the inventory: "));
        int co=0;
        while(prodName[co] !='\0')
        {
            co++;
        }
        int co1=0;
        while(old_item.productName[co1] !='\0')
        {
            co1++;
        }
        write(log_fd, old_item.productName, co1);
        write(log_fd, " ", sizeof(" "));
        write(log_fd, prodName, co);
        write(log_fd, "\n\r", sizeof("\n"));

        lockl.l_type=F_UNLCK;    
        fcntl(log_fd,F_SETLK,&lockl);  
        printf("UNlocked the log file\n");
        

    return SO_SUCCESS;
}

int inv_delete(int prodID)
{
    struct InventoryItem item,old_item;
    int not_found = 1;
    int offset = lseek(inv_handle.inv_data_fd,0,SEEK_END);
    int size_inv = sizeof(struct InventoryItem);
    int go_back_offset = -1;

    struct flock lock;  //setting a read lock on the whole file to check whether that product ID already has been added
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=offset;
    fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);

    // printf("Setting the read lock.\n");

    lseek(inv_handle.inv_data_fd,0,SEEK_SET); 
    while(read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem)) == size_inv)
    {
        // printf("ProductID = %d\n",item.productID);
        if(item.valid == 1)
        {
            if(item.productID == prodID)
            {  
                not_found = 0;
                go_back_offset = sizeof(struct InventoryItem);
                offset = lseek(inv_handle.inv_data_fd,-go_back_offset,SEEK_CUR);
                old_item.productID = item.productID;
                strcpy(old_item.productName,item.productName);
                old_item.price = item.price;
                break;
            }
        }   
    }

    lock.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
    // printf("Released the read lock.\n");

    // item was not found
    if(not_found == 1)
    {
        return INVENTORY_ITEM_NOT_FOUND;
    }
    
    old_item.valid = 0;
    
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=offset;
    lock.l_len=sizeof(struct InventoryItem);
    fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
    // printf("Locked for writing.\n");

    lseek(inv_handle.inv_data_fd,offset,SEEK_SET);
    write(inv_handle.inv_data_fd,&old_item,sizeof(struct InventoryItem));
    
    lock.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
    // printf("Released the write lock.\n");

    struct InventoryCount count_buf,count;
    int tasks_done = 0;
    strcpy(count.prodName,old_item.productName);

    int num=0; //for the quantity in the quantity count file
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=lseek(inv_handle.inv_quantity_fd,0,SEEK_END);
    fcntl(inv_handle.inv_quantity_fd,F_SETLKW,&lock);
    lseek(inv_handle.inv_quantity_fd,0,SEEK_SET);
    while(read(inv_handle.inv_quantity_fd,&count_buf,sizeof(struct InventoryCount)))
    {
        if(!strcmp(count_buf.prodName,old_item.productName))   
        {
            lseek(inv_handle.inv_quantity_fd,-sizeof(struct InventoryCount),SEEK_CUR);    //decreasing the stored quantity
            num = count_buf.quantity;
            count.quantity = num - 1;
            if(write(inv_handle.inv_quantity_fd,&count,sizeof(struct InventoryCount)) == -1)
            {
                printf(("Quantity could not be updated in the Keepcount file\n"));
            }
            else
            {
                printf("Quantity could successfully be updated in the Keepcount file\n");
            }
            break;
        }
        
    }

    lock.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lock);  //unlock

        log_fd = log_open();
        lockl.l_type=F_WRLCK;
        lockl.l_whence=SEEK_SET;
        lockl.l_start=0;
        lockl.l_len=lseek(log_fd,0,SEEK_END);
        fcntl(log_fd,F_SETLKW,&lockl);

        

        
        lseek(log_fd, 0, SEEK_END);
        write(log_fd, "Product deleted from the inventory: ", sizeof("Product deleted to the inventory: "));
        int co=0;
        while(old_item.productName[co] !='\0')
        {
            co++;
        }
        
        write(log_fd, old_item.productName, co);
        write(log_fd, "\n\r", sizeof("\n"));

        lockl.l_type=F_UNLCK;    
        fcntl(log_fd,F_SETLK,&lockl);  
        

    return SO_SUCCESS;

}

int inv_display(int nsd)
{
    if( inv_handle.inv_status == INVENTORY_CLOSED)
	{
        printf("Cannot display inventory\n");
		return SO_FILE_ERROR;
	}
    else
    {
        struct InventoryItem item;
        struct flock lock;  //setting a read lock on the whole file to check whether that product ID already has been added
        lock.l_type=F_RDLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=0;
        lock.l_len=lseek(inv_handle.inv_data_fd,0,SEEK_END);

        fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);   

        // printf("Setting the read lock.\n");

        lseek(inv_handle.inv_data_fd,0,SEEK_SET); //taking the cursor to the beginning
        int i = 0;

        // displaying
        while(read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem)))
        {
            if(item.valid == 1)
            {  
                write(nsd,&item,sizeof(struct InventoryItem));  
            }
            i++;
            if(i == MAX_INVENTORY_SIZE)
            {
                break;
            }
        }

        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);
        // printf("Released the read lock.\n");

        return SO_SUCCESS;
    }
}

int inv_close()
{
    if( inv_handle.inv_status == INVENTORY_CLOSED)
	{
		return SO_FILE_ERROR;
	}
	else
    {
        //closing files
        close(inv_handle.inv_data_fd);
        close(inv_handle.inv_quantity_fd);
		inv_handle.inv_status = INVENTORY_CLOSED;
		return SO_SUCCESS;
    }
}


int cart_create()
{
    int cart_fd = open("Cart.dat",O_RDONLY | O_CREAT,0644);
    if(cart_fd == -1)
    {
        perror("Error in creating the cart file.\n");
        return SO_FILE_ERROR;
    }

    close(cart_fd);
}

int cart_open()
{
    int cart_fd = open("Cart.dat",O_RDWR,0644);

    if(cart_fd == -1)
    {
        perror("Error in opening the cart file.\n");
        return SO_FILE_ERROR;
    }

    if (cart_handle.cart_status  == INVENTORY_OPEN)
    {
        return INVENTORY_ALREADY_OPEN;
    }
    else
    {
    	cart_handle.cart_entry_size = sizeof(struct Cart);
    	cart_handle.cart_data_fd = cart_fd;
        cart_handle.cart_status = CART_OPEN;
        cart_handle.cart_count = 0; //GET RID OF THIS IF IT'S NOT BEING USED

	}

}

int cart_display(int custID, int nsd)
{
    int size=0; // number of products in the cart
    struct Cart tmp;
    int getResult = cart_get_rec(custID, &tmp);


    if(getResult == 0)
    {
        lseek(inv_handle.inv_data_fd,0,SEEK_SET); //taking the cursor to the beginning

        
        write(nsd,&tmp,sizeof(struct Cart));   //this is writing to the socket not the file 
      
    }
    else if(getResult == 8)
    {

        strcpy(tmp.cartItems[0].prodName, "No item");
        tmp.cartItems[0].price=0;
        tmp.cartItems[0].quantity=0;
        tmp.noOfProducts=1;
        write(nsd,&tmp,sizeof(struct Cart));   //this is writing to the socket not the file
        
    }
    
}

int cart_add(int custID,char pname[],int price, int quan)
{
    struct Cart temp;
    int found=0, size=0;  //to indicate if the product already exists in the cart
    int getResult = cart_get_rec(custID, &temp);

    if( getResult == 0)
    {

        //go through the products to see if this product name already exists
        for(int j=0; j<temp.noOfProducts;j++)
        {
            if(strcmp(temp.cartItems[j].prodName, pname) == 0)
            {
                printf("The product name already exists\n");
                found = 1;
                int num = temp.cartItems[j].quantity + quan;
                temp.cartItems[j].quantity = num;
                temp.valid = 1;
                cart_update(custID, &temp);
            }
        } 
        if(found == 0)  // product name not found in the cart
        {

            size = temp.noOfProducts;

            // printf("size of pname = %ld\n", sizeof(pname)/sizeof(pname[0]));
            strcpy(temp.cartItems[size].prodName, pname);
            temp.cartItems[size].price=price;
            temp.cartItems[size].quantity=quan;
            int number=0;
            number=temp.noOfProducts+1;
            temp.noOfProducts=number;  //increasing the number of entries in the Cart
            temp.valid = 1;
            cart_update(custID, &temp);
        }
    }
    else if(getResult == 8) //REC_NOT_FOUND (customer id not found)
    {

        struct Cart rec;
        size = temp.noOfProducts;
        strcpy(rec.cartItems[0].prodName, pname);
        rec.cartItems[0].price = price;
        rec.cartItems[0].quantity = quan;
        rec.custID = custID;
        rec.noOfProducts = 1;
        rec.valid = 1;
        cart_put_rec(custID, &rec);

    } 
}

int cart_edit(int custID, char pname[], int quan)
{
    struct Cart temp;
    int found = 0; //to indicate if a product is found
    int getResult = cart_get_rec(custID, &temp);
    if( getResult == 0)  // the customer id exists
    {
        //go through the products to see if this product name already exists
        for(int j=0; j<temp.noOfProducts;j++)
        {
            if(strcmp(temp.cartItems[j].prodName, pname) == 0)
            {

                found = 1;
                temp.cartItems[j].quantity = quan;
                cart_update(custID, &temp);
            }
        } 
        if(found == 0)
        {
            printf("The product does not exist.\n");
        }  
    } 
    else
    {
        printf("The customer ID does not exist.\n");
    }
}

int cart_delete_item(int custID, char pname[])
{
    struct Cart temp;
    int getResult = cart_get_rec(custID, &temp);
    int number; //temporary variable
    if( getResult == 0)  // the customer id exists
    for(int j=0; j<temp.noOfProducts;j++)
    {
        if(strcmp(temp.cartItems[j].prodName, pname) == 0)
        {
           for(int i=j;i<temp.noOfProducts-1;i++)
           {
                temp.cartItems[i]=temp.cartItems[i+1];
           } 
           number = temp.noOfProducts;
           temp.noOfProducts=number - 1;
        }
    } 
    cart_update(custID, &temp);

}

int cart_put_rec( int id, void *rec ){

    int offset=0;  // used for file locking
	if(cart_handle.cart_status == CART_CLOSED)
    {
    	return SO_FILE_ERROR;
    }
   	else
    {
        struct Cart buf;
        lseek(cart_handle.cart_data_fd, 0, SEEK_SET);

        struct flock lock1;
        lock1.l_type=F_WRLCK;
        lock1.l_whence=SEEK_SET;
        lock1.l_start=0;
        lock1.l_len=lseek(cart_handle.cart_data_fd, 0, SEEK_END);
        fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock1);
        
    
        while(read(cart_handle.cart_data_fd, &buf, sizeof(buf)))
        {
            if(buf.custID == id)
            {
                if(buf.valid != 0)
                {
                    printf("Failed\n");  // the cart already exists
		   		    return CART_ADD_FAILED;
                }
                else
                {
                    // struct flock lock;  //setting a lock on the record where I want to write
                    // lock.l_type=F_WRLCK;
                    // lock.l_whence=SEEK_SET;
                    // lock.l_start=lseek(cart_handle.cart_data_fd, 0, SEEK_CUR);
                    // lock.l_len=sizeof(struct Cart);
                    // fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock); //write lock 
                    lseek(cart_handle.cart_data_fd, -sizeof(struct Cart), SEEK_CUR);
                    write(cart_handle.cart_data_fd, rec, cart_handle.cart_entry_size);

                    //unlock
                    lock1.l_type=F_UNLCK;
                    fcntl(cart_handle.cart_data_fd, F_SETLK,&lock1);  
                    return SO_SUCCESS;
                }
                
            }
        }

        lock1.l_type=F_UNLCK;
        fcntl(cart_handle.cart_data_fd,F_SETLK,&lock1);

        int wrote=0;
        //if it came here, that means the customer id does not exist
        

        struct flock lock;  //setting a lock on the record where I want to write
        lock.l_type=F_WRLCK;
        lock.l_whence=SEEK_SET;
        lock.l_start=lseek(cart_handle.cart_data_fd, 0, SEEK_END);  // to write to the end of the file
        lock.l_len=sizeof(struct Cart);
        fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock); //write lock 

        wrote = write(cart_handle.cart_data_fd, rec, cart_handle.cart_entry_size);

        lock.l_type = F_UNLCK;
        fcntl(cart_handle.cart_data_fd,F_SETLK,&lock);  //unlock
        if(wrote == -1)
        {
            return CART_NDX_SAVE_FAILED;
        }
        else
        {
            return SO_SUCCESS;
        }


    	
	}

}

int cart_get_rec( int id, struct Cart *rec )
{
	 if(cart_handle.cart_status == CART_CLOSED)
    	{

        	return SO_FILE_ERROR;
    	}
    	else
    	{
            int count=0;

            struct Cart buf;

            lseek(cart_handle.cart_data_fd, 0, SEEK_SET);

            struct flock lock1;

            lock1.l_type=F_RDLCK;
            lock1.l_whence=SEEK_SET;
            lock1.l_start=0;

            lock1.l_len=lseek(cart_handle.cart_data_fd, 0, SEEK_END);
            fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock1);

            lseek(cart_handle.cart_data_fd, 0, SEEK_SET);
            
            while(read(cart_handle.cart_data_fd, &buf, sizeof(struct Cart)) && count<MAX_CARTS)
            {
                if(buf.custID == id)
                {
                    if(buf.valid != 1)
                    {
                        printf("Failed\n");  // the cart already exists
		   	    	    return CART_ITEM_NOT_FOUND;
                    }
                    else
                    {

                        // read(cart_handle.cart_data_fd, rec, cart_handle.cart_entry_size);
                        rec->custID = buf.custID;
                        rec->noOfProducts = buf.noOfProducts;
                        rec->valid = buf.valid;
                        for(int i=0;i<buf.noOfProducts;i++)
                        {
                            rec->cartItems[i] = buf.cartItems[i];
                        }


                        lock1.l_type=F_UNLCK;
                        fcntl(cart_handle.cart_data_fd,F_SETLK,&lock1);

                        return SO_SUCCESS;
                    }

                }
                count++;
            }


            lock1.l_type=F_UNLCK;
            fcntl(cart_handle.cart_data_fd,F_SETLK,&lock1);


            

            

        return CART_ITEM_NOT_FOUND;

        }	
		
}

int cart_update(int id, void *newrec)
{
    if(cart_handle.cart_status == CART_CLOSED)
    {
        return SO_FILE_ERROR;
    }
    else
    {
        struct flock lock1;
        lock1.l_type=F_WRLCK;
        lock1.l_whence=SEEK_SET;
        lock1.l_start=0;
        lock1.l_len=lseek(cart_handle.cart_data_fd, 0, SEEK_END);
        fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock1);
        struct Cart buf;

        lseek(cart_handle.cart_data_fd, 0, SEEK_SET);
        while(read(cart_handle.cart_data_fd, &buf, sizeof(buf)))
        {
            if(buf.custID == id)
            {
                if(buf.valid != 1)
                {
                    printf("Failed\n");  // the cart already exists
		   		    return CART_ITEM_NOT_FOUND;
                }
                else
                {

                    lseek(cart_handle.cart_data_fd, -sizeof(struct Cart), SEEK_CUR);

                    write(cart_handle.cart_data_fd, newrec, cart_handle.cart_entry_size);
                    lock1.l_type=F_UNLCK;
                    fcntl(cart_handle.cart_data_fd, F_SETLK,&lock1);


                    return SO_SUCCESS;
                }
                
            }
        }

        lock1.l_type=F_UNLCK;
        fcntl(cart_handle.cart_data_fd, F_SETLK,&lock1);

        return CART_ITEM_NOT_FOUND;
    }
}

int cart_close()
{
    if( cart_handle.cart_status == CART_CLOSED)
	{
		return SO_FILE_ERROR;
	}
	else
	{
		close(cart_handle.cart_data_fd);
        close(cart_handle.cart_ndx_fd);
		cart_handle.cart_status = CART_CLOSED;
		return SO_SUCCESS;
	}  
}


int buyItems(int custID,int nsd)
{
    struct Cart cart;
    int i=0;
    int totalcost=0;    //this will store the total cost for the cart


    //reading the cart data file to find this customer's cart
    struct flock lock;  //setting a read lock on the whole file to check whether that product ID already has been added
    struct flock lockc;
    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=lseek(cart_handle.cart_data_fd,0,SEEK_END);
    fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock);

    lseek(cart_handle.cart_data_fd,0,SEEK_SET);
    while(read(cart_handle.cart_data_fd,&cart,sizeof(struct Cart)))    //to find the customer id
    {
        if(cart.custID == custID)
        {
            break;
        }

        i++;

        if(i == MAX_CARTS)
        {
            break;
        }
    }

    lock.l_type=F_UNLCK;    
    fcntl(cart_handle.cart_data_fd,F_SETLK,&lock);



    if(i == MAX_CARTS)
    {
        return BUY_FAILED;
    }


    //writing to the inventory file with the changes
    lock.l_type=F_WRLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=lseek(inv_handle.inv_data_fd,0,SEEK_END);
    fcntl(inv_handle.inv_data_fd,F_SETLKW,&lock);
    printf("Write locked in buy\n");

    //checking the quantity log to check whether enough quantity of items are present
    struct InventoryCount count;

    lockc.l_type=F_RDLCK;
    lockc.l_whence=SEEK_SET;
    lockc.l_start=0;
    lockc.l_len=lseek(inv_handle.inv_quantity_fd,0,SEEK_END);
    fcntl(inv_handle.inv_quantity_fd,F_SETLKW,&lockc);

    lseek(inv_handle.inv_quantity_fd,0,SEEK_SET);
    int j = 0,found = 0;


    for(int i=0;i<cart.noOfProducts;i++)
    {
        found = 0;
        j = 0;
        lseek(inv_handle.inv_quantity_fd,0,SEEK_SET);
        while(read(inv_handle.inv_quantity_fd,&count,sizeof(struct InventoryCount)))
        {
 
            if(!strcmp(count.prodName,cart.cartItems[i].prodName) && count.quantity < cart.cartItems[i].quantity)
            {

                return BUY_FAILED;
            }

            if(!strcmp(count.prodName,cart.cartItems[i].prodName))
            {
                found = 1;
                break;
            }

            j++;

            if(j == MAX_CART_SIZE)
            {
                break;
            }
        }
        if(found != 1)
        {
            printf("Couldn't find %s at all.\n",count.prodName);
            lock.l_type=F_UNLCK;    
            fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);  //unlock
            lockc.l_type=F_UNLCK;    
            fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lockc);
            write(nsd,&totalcost,sizeof(int));  //sending totalcost to the customer. can proceed to payment. On the customer side the intrface to pay will appear, and this function will wait for approval to move on.
            return BUY_FAILED;
        }

    }

    lockc.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lockc);
    // printf("Released the read lock.\n");

   


    //computing the total cost for the customer
    int price=0, q=0;   //price and quantity of the cart items in the loop



    for(int i=0;i<cart.noOfProducts;i++)
    {
        price = cart.cartItems[i].price;
        q = cart.cartItems[i].quantity;
        totalcost=totalcost+(price * q);

    }

    printf("totalcost = %d\n", totalcost);

    if(totalcost == 0)
    {
        lock.l_type=F_UNLCK;    
        fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);  //unlock
        write(nsd,&totalcost,sizeof(int));  //sending totalcost to the customer. can proceed to payment. On the customer side the intrface to pay will appear, and this function will wait for approval to move on.

        printf("Unlocked the inventory\n");
        return BUY_FAILED;

    }

    cart_display(custID,nsd);
    
    write(nsd,&totalcost,sizeof(int));  //sending totalcost to the customer. can proceed to payment. On the customer side the intrface to pay will appear, and this function will wait for approval to move on.
    char buf[2];
    read(nsd,&buf,sizeof(buf));  //get confirmation of the payment

    if(strcmp(buf,"0"))
    {
        return BUY_FAILED;
    }

   


    //finding the items in the inventory to be deleted
    struct InventoryItem item;
    //int to_be_deleted[cart.noOfProducts];
    int index_to_be_deleted[cart.noOfProducts];
    int k = 0,no_of_loops = 0;



    
    for(int i=0;i<cart.noOfProducts;i++)
    {
        found = 0;
        no_of_loops = 0;
        lseek(inv_handle.inv_data_fd,0,SEEK_SET);
        
        for(int j = 0;j<cart.cartItems[i].quantity;j++)
        {
            while(read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem)))
            {
                no_of_loops = no_of_loops + 1;
                if(!strcmp(cart.cartItems[i].prodName,item.productName) && item.valid == 1)
                {
                    //to_be_deleted[k] = item.productID;
                    index_to_be_deleted[k] = no_of_loops - 1;
                    
                    k = k+1;
                    
                    break;
                }
                
            
                if(no_of_loops == MAX_INVENTORY_SIZE)
                {
                    break;
                }
            }
        }  

    }


    // printf("Unlocked the read lock.\n");
    printf("k = %d\n", k);

    


    int offset=-1;

    for(int i=0;i<k;i++)
    {

        offset = index_to_be_deleted[i]*sizeof(struct InventoryItem);
        lseek(inv_handle.inv_data_fd, offset,SEEK_SET);
        read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem));
        
        item.valid = 0;
        
        lseek(inv_handle.inv_data_fd,-sizeof(struct InventoryItem),SEEK_CUR);
        write(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem));

    }

    struct InventoryCount count1[MAX_INVENTORY_SIZE];

    for(int i=0;i<MAX_INVENTORY_SIZE;i++)
    {
        strcpy(count1[i].prodName,"");
        count1[i].quantity = 0;
    }

    i = 0;
    int index = 0;
    
    lseek(inv_handle.inv_data_fd,0,SEEK_SET);
    while(read(inv_handle.inv_data_fd,&item,sizeof(struct InventoryItem)))
    {
        if(item.valid == 1)
        {
            index = inCount(item.productName,count1);
            if(index != -1)
            {
                count1[index].quantity++;
            }
            else
            {
                for(int j = 0;j<MAX_INVENTORY_SIZE;j++)
                {
                    if(!strcmp(count1[j].prodName,""))
                    {
                        strcpy(count1[j].prodName,item.productName);
                        count1[j].quantity = 1;
                        break;
                    }
                }
            }
            
            i++;
    
            if(i == MAX_INVENTORY_SIZE)
            {
                break;
            } 

        }
            
    }


    lockc.l_type=F_WRLCK;
    lockc.l_whence=SEEK_SET;
    lockc.l_start=0;
    lockc.l_len=lseek(inv_handle.inv_quantity_fd,0,SEEK_END);
    fcntl(inv_handle.inv_quantity_fd,F_SETLKW,&lockc);

    lseek(inv_handle.inv_quantity_fd,0,SEEK_SET);
    for(int i=0;i<MAX_INVENTORY_SIZE;i++)
    {
        write(inv_handle.inv_quantity_fd,&count1[i],sizeof(struct InventoryCount));    
    }

    lockc.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_quantity_fd,F_SETLK,&lockc);  //unlock
    





    lock.l_type=F_UNLCK;    
    fcntl(inv_handle.inv_data_fd,F_SETLK,&lock);  //unlock
    printf("Unlocked the inventory\n");

    // emptying the cart
    struct Cart cart_buf, new_cart;
    int c = 0;

    lock.l_type=F_RDLCK;
    lock.l_whence=SEEK_SET;
    lock.l_start=0;
    lock.l_len=lseek(cart_handle.cart_data_fd,0,SEEK_END);
    fcntl(cart_handle.cart_data_fd,F_SETLKW,&lock);
    // printf("Locked for reading.\n");

    lseek(cart_handle.cart_data_fd,0,SEEK_SET);
    while(read(cart_handle.cart_data_fd,&cart_buf,sizeof(struct Cart)))
    {
        if(cart_buf.custID == custID)
        {
            offset = lseek(cart_handle.cart_data_fd,-sizeof(struct Cart),SEEK_CUR);
            break;
        }
        c++;
        if(c == MAX_CARTS)
        {
            break;
        }
    }

    
    

    new_cart.custID  = custID;
    new_cart.noOfProducts = 0;
    new_cart.valid = 1;

    for(int i=0;i<MAX_CART_SIZE;i++)
    {
        new_cart.cartItems[i].quantity = 0;
        strcpy(new_cart.cartItems[i].prodName,"");
        new_cart.cartItems[i].price = 0;
    }

    lock.l_type=F_UNLCK;    
    fcntl(cart_handle.cart_data_fd,F_SETLK,&lock);  //unlock
    // printf("Unlocked the read lock.\n");
    
    //writing the empty cart back to the cart file
    lockc.l_type=F_WRLCK;
    lockc.l_whence=SEEK_SET;
    lockc.l_start=offset;
    lockc.l_len=sizeof(struct Cart); 
    fcntl(cart_handle.cart_data_fd,F_SETLKW,&lockc);
    // printf("Locked for writing.\n");

    lseek(cart_handle.cart_data_fd,offset,SEEK_SET);
    write(cart_handle.cart_data_fd,&new_cart,sizeof(struct Cart));

    lockc.l_type=F_UNLCK;    
    fcntl(cart_handle.cart_data_fd,F_SETLK,&lockc);  //unlock
    // printf("Unlocked the write lock.\n");




    return SO_SUCCESS;

}




int main()
{
    inv_create();
    inv_open();
    cart_create();
    cart_open();
    log_create();
    log_open();

    // printf("Inventory successfully setup.\n");
    //setting up the socket
    struct sockaddr_in serv,cli; 
    

    int sd = socket(AF_INET,SOCK_STREAM,0);
    if (sd == -1){
        perror("Error: ");
        return -1;
    }
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORTNO);

    int opt = 1;
    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))){
        perror("Error: ");
        return -1;
    }

    if(bind(sd,(struct sockaddr *)&serv,sizeof(serv)) == -1)
    {
        perror("Bind error:\n");
        return -1;
    }

    if(listen(sd,100) == -1)
    {
        perror("Listen erro:\n");
        return -1;
    }

    int nsd;
    int size = sizeof(cli);
    int option;

    while(1)
    {
        // // printf("Type\n1) To wait for more requests\n2) To exit\n");
        // int ans;
        // // scanf("%d",&ans);
        
        // if(ans == 2)
        // {
        //     close(sd);
        //     printf("Closing the store.\n");
        //     inv_close();
        //     ans=0;  //resetting
        //     exit(0);
        // }

            nsd = accept(sd,(struct sockaddr *)&cli,&size);

            if(nsd != -1)
            {
                printf("Connection accepted\n");
                
                if(!fork())
                {
                   
                    // close(sd);

                    read(nsd,&option,sizeof(option));
                    
                    if(option == 1)
                    {
                        // printf("Asked to display the inventory\n");
                        inv_display(nsd);
                    }
                    else if(option == 2)
                    {
                        
                        struct InventoryItem item;
                        char response[100];
                        read(nsd,&item,sizeof(struct InventoryItem));
                        
                        if(inv_add(item.productID,item.productName,item.price) == SO_SUCCESS)
                        {
                            strcpy(response,"Successfully added the product.");
                        }
                        else
                        {
                            strcpy(response,"Could not add the product. Sorry!");
                        }
                        write(nsd,response,sizeof(response));

                        
                        
                    }
                    else if(option == 3)
                    {

                        struct InventoryItem item;
                        char response[100];
                        read(nsd,&item,sizeof(struct InventoryItem));
                        printf("ProdID = %d, ProductName = %s, Price = %d\n",item.productID,item.productName,item.price);
                        if(inv_update(item.productID,item.productName,item.price) == SO_SUCCESS)
                        {
                            strcpy(response,"Successfully updated the product.");
                        }
                        else
                        {
                            strcpy(response,"Could not update the product. Sorry!");
                        }
                        write(nsd,response,sizeof(response));
                        
                        

                        
                    }
                    else if(option == 4)
                    {
                        char response[100];
                        int prodID;
                        read(nsd,&prodID,sizeof(prodID));
                        // printf("ProductID = %d\n",prodID);

                        if(inv_delete(prodID) == SO_SUCCESS)
                        {
                            strcpy(response,"Successfully deleted the product.");
                        }
                        else
                        {
                            strcpy(response,"Could not delete the product. Sorry!");
                        }
                        printf("Response: %s\n",response);
                        write(nsd,response,sizeof(response));
                        

                        
                    }
                    else if(option == 5)   //customer options
                    {
                        
                        int custID;
                        read(nsd,&custID,sizeof(int));    //taking in the customer ID
                        
                        cart_display(custID, nsd);
                    }
                    else if(option == 6)
                    {
                        int custID;
                        int quan=0, price=0;
                        char pname[100];

                        read(nsd,&custID,sizeof(int));    //taking in the customer ID

                        cart_display(custID, nsd);
                        // now read the product name, price and quantity and pass it to the add function
                        read(nsd,&pname,sizeof(pname));    //taking in the product name
                        
                        read(nsd,&price,sizeof(int));    //taking in the price
                        read(nsd,&quan,sizeof(int));    //taking in the quantity

                        cart_add(custID, pname, price, quan);
                    }
                    else if(option == 7)
                    {
                        int custID;
                        int opt;
                        read(nsd,&custID,sizeof(int));    //taking in the customer ID
                        
                        //displaying the cart for the customer
                        cart_display(custID, nsd);
                        
                        read(nsd,&opt,sizeof(int)); 
                        if(opt == 1)
                        {
                            char pname[100];
                            int quan=0;

                            read(nsd, &pname, sizeof(pname));
                            read(nsd, &quan, sizeof(int));
                            cart_edit(custID, pname, quan);
                        }   
                        if(opt == 2)
                        {
                            char pname[100];
                            read(nsd, &pname, sizeof(pname));
                            cart_delete_item(custID, pname);
                        }

                    }
                    else if(option == 8)
                    {
                        inv_display(nsd);
                    }
                    else if(option == 9)
                    {
                        int custID;
                        read(nsd,&custID,sizeof(int));    //taking in the customer ID
                        // printf("cust ID = %d\n", custID);

                        buyItems(custID,nsd);
                    }
                    // ans=0;  //resetting
                    close(nsd);
                
                }
                else
                {
                    // ans=0;
                    close(nsd);
                }
            }
            else
            {
                printf("Unsuccessful accept\n");
                perror("socket");
                break;
            }
            
        
    }

    return 0;
}

