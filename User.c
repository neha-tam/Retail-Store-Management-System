#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>

#include "Server.h"

#define PORTNO 8004

// #include "pds.h"

struct InventoryItem Cart[MAX_CART_SIZE];




int main()
{
    int custID=0;
    printf("Enter your Customer ID: ");
    scanf("%d", &custID);
    printf("Welcome to the store!\n");
    printf("What would you like to do today?\n");


     while(1)
    {
    
    
    struct sockaddr_in serv;
    int sd = socket(AF_INET,SOCK_STREAM,0);

    if (sd == -1){
        perror("Error: ");
        return -1;
    }

    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(PORTNO);
    
    int option=0, opt=0; //to store which option was chosen, option in the edit part, customer id
    int pid=0, quan=0, price=0;
    char pname[100];
    char ch; //dummy character
    int size=0;  //number of products in the cart


    if(connect(sd,(struct sockaddr *)&serv,sizeof(serv)) != -1)
    {

        
        printf("Type 1 to display the cart\nType 2 to add items to the cart\nType 3 to edit the cart\nType 4 to view the inventory\nType 5 to proceed to pay\n");
        scanf("%d", &option);
        option=option+4;  //to sync with the option numbers in the server program
        
        write(sd, &option, sizeof(int));
        if(option == 5)
        {
            
            // DisplayCart();
            write(sd, &custID, sizeof(int));  //sending the customer ID

            struct Cart tmp;
            printf("ProductName   Price   Quantity\n");
            // sleep(5);
            read(sd,&tmp,sizeof(struct Cart));
                
            size=tmp.noOfProducts;
            
            for(int i=0;i<size;i++)
            {
                printf("%s\t\t%d\t\t%d\n", tmp.cartItems[i].prodName, tmp.cartItems[i].price, tmp.cartItems[i].quantity);  
            }
                


        }
        else if(option == 6)
        {
            
            write(sd, &custID, sizeof(int));  //sending the customer ID

            struct Cart tmp;
            printf("ProductName   Price   Quantity\n");
            // sleep(5);
            read(sd,&tmp,sizeof(struct Cart));
                
            size=tmp.noOfProducts;
            
            for(int i=0;i<size;i++)
            {
                printf("%s\t\t%d\t\t%d\n", tmp.cartItems[i].prodName, tmp.cartItems[i].price, tmp.cartItems[i].quantity);  
            }

            int found = 0; //a variable to indicate whether this productName already exists in the Cart or not
            printf("Enter the product name: ");
            scanf("%s", pname);
            printf("Enter the price: ");
            scanf("%d", &price);
            printf("Enter the quantity: ");
            scanf("%d", &quan);


            

            write(sd, &pname, sizeof(pname));  //sending the customer ID
            write(sd, &price, sizeof(int));  //sending the customer ID
            write(sd, &quan, sizeof(int));  //sending the customer ID
            


        }
        else if(option == 7)
        {
            int opt, done = 0;  //dine is for indicating that displaying the cart is done
            //display the cart
          
            write(sd, &custID, sizeof(int));  //sending the customer ID

            struct Cart tmp;
            printf("ProductName   Price   Quantity\n");
            read(sd,&tmp,sizeof(struct Cart));
                         
            size=tmp.noOfProducts;

            for(int i=0;i<size;i++)
            {
                printf("%s\t\t%d\t\t%d\n", tmp.cartItems[i].prodName, tmp.cartItems[i].price, tmp.cartItems[i].quantity);  
            }



            printf("Do you want to\n1) Edit the quantity of a product OR\n2) Delete a product\n");
            scanf("%d", &opt);


            if(opt == 1)
            {
                // edit
                int choice=1;
                char pname[100];
                int quan=0;
                write(sd, &choice, sizeof(int));  //telling the server what to do
                printf("Enter the name of product whose quantity you want to change:");
                scanf("%s", pname);
                printf("Enter the new quantity of the product: ");
                scanf("%d", &quan);

                write(sd, &pname, sizeof(pname));
                write(sd, &quan, sizeof(int));

            }
            else if(opt == 2)
            {
                // delete
                int choice=2;
                char pname[100];
                write(sd, &choice, sizeof(int));  //telling the server what to do
                printf("Enter the name if the product you want to delete from the cart: ");
                scanf("%s", pname);

                write(sd, &pname, sizeof(pname));



            }
            else
            {
                printf("Please enter 1 or 2.\n");
            }




        }
        else if(option == 8)
        {
            // check inventory
            struct InventoryItem item;
            while(read(sd,&item,sizeof(struct InventoryItem)))
            {

                printf("%d\t%s\t%d\n",item.productID,item.productName,item.price);
            }
        }
        else if(option == 9)
        {
            // buy items
            int totalcost=0; //total cost to be payed4
            int money=0;
            write(sd, &custID, sizeof(int));

            printf("Your cart:\n");
            struct Cart tmp;
            printf("ProductName   Price   Quantity\n");
            // sleep(5);
            read(sd,&tmp,sizeof(struct Cart));
                
            size=tmp.noOfProducts;

            for(int i=0;i<size;i++)
            {
                printf("%s\t\t%d\t\t%d\n", tmp.cartItems[i].prodName, tmp.cartItems[i].price, tmp.cartItems[i].quantity);  
            }

            
            if(read(sd, &totalcost, sizeof(int)) == -1)
            {
                printf("Purchase failed\n");
            }
            else
            {
                if(totalcost == 0)
                {
                   printf("Purchase failed\n"); 
                   write(sd, "1", sizeof("1"));
                }
                else
                {
                printf("The total cost is = %d\nPlease enter the amount to pay for the items bought:", totalcost );
                scanf("%d", &money);
                if(money == totalcost)
                {
                    printf("Paid.\n");



                    write(sd, "0", sizeof("0"));
                }
                }

                
            }

        }
        else
        {
            printf("Please enter 1, 2, or 3\n");
        }
        }

        else
    {
        printf("Connection failed\n");
        perror("socket");
    }

    close(sd);
        
    }
    
     
    

    return 0;
}