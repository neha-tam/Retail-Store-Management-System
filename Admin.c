#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

#include "Server.h"

#define PORTNO 8004

int main()
{
    printf("Welcome!\n");
    while (1)
    {
    struct sockaddr_in serv;
    int server_fd = socket(AF_INET,SOCK_STREAM,0);

    if (server_fd == -1)
    {
        perror("Error: ");
        return -1;
    }

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORTNO);
    
    
    
    if(connect(server_fd,(struct sockaddr *)&serv,sizeof(serv)) != -1)
    {
       
        
        
        printf("Type 1 to check the inventory\nType 2 to add a product\nType 3 to update an item in the inventory\nTYpe 4 to delete an item\n"); 
        int option;
        scanf("%d",&option);
        int size = sizeof(option);
        write(server_fd,&option,size);
        if(option == 1)
        {
            struct InventoryItem inv;
            while(read(server_fd,&inv,sizeof(struct InventoryItem)))
            {
                printf("%d\t%s\t%d\n",inv.productID,inv.productName,inv.price);
            }
        }
        else if(option == 2)
        {
            int prodID;
            char prodName[100];
            int price;
            printf("Enter the product name: ");
            scanf("%s",prodName);
            printf("Enter the product ID: ");
            scanf("%d",&prodID);
            printf("Enter the product price: ");
            scanf("%d",&price);
            
            struct InventoryItem inv ;
            inv.productID = prodID;
            strcpy(inv.productName,prodName);
            inv.price = price;
            write(server_fd,&inv,sizeof(inv));
            char feedback[200];
            read(server_fd,feedback,sizeof(feedback));
            printf("%s\n",feedback);
        }
        else if(option == 3)
        {
            printf("in option 3\n");
            int prodID;
            char prodName[100];
            int price;
            printf("Enter the product ID of the product to be updated (updated based on productID): \n");
            scanf("%d",&prodID);
            printf("Enter the updated name (if you wish to update the name) or enter the same:\n");
            scanf("%s",prodName);
            printf("Enter the updated price (if you wish to update the price) or add the same price:\n");
            scanf("%d",&price);

            struct InventoryItem item ;
            item.productID = prodID;
            strcpy(item.productName,prodName);
            item.price = price;
            write(server_fd,&item,sizeof(item));
            char response[100];
            read(server_fd,response,sizeof(response));
            printf("%s\n",response);
        }
        else if(option == 4)
        {
            printf("Enter the productID you would like to remove:\n");
            int prodID;
            scanf("%d",&prodID);
            printf("ProductID = %d\n",prodID);
            write(server_fd,&prodID,sizeof(prodID));
            char response[100];
            read(server_fd,response,sizeof(response));
            printf("%s\n",response);
        }
        
    }
    else
    {
        printf("Could not connect.\n");
        perror("");
    }
    
    
    

    close(server_fd);
    }
    
    
    
    
}