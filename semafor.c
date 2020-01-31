#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>

#define MAX_QUEUE 4
#define ADMIN_P 2
#define VIP_P 1
#define USER_P 0

struct MESSAGE
{
    int priority;
    char* text;
    struct MESSAGE* next_mess;
};

static unsigned int mess_num = 0;

struct MESSAGE* user_queue; //wskaźnik na pierwszy element listy user
struct MESSAGE* vip_queue;  //wskaźnik na pierwszy element listy vip
struct MESSAGE* last_vip;
struct MESSAGE* last_user;
pthread_mutex_t user_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t vip_queue_mutex = PTHREAD_MUTEX_INITIALIZER;

sem_t empty;
sem_t full;

void init_queue()
{
    user_queue = NULL;
    vip_queue = NULL;
    last_user = NULL;
    last_vip = NULL;
    sem_init(&empty, 0, MAX_QUEUE);
    sem_init(&full, 0, 0);
}

void print_queue(struct MESSAGE* queue)  
{
    struct MESSAGE* msg;
    msg = queue;
    while(msg)
    {
        if(msg->priority == VIP_P)
            printf("[VIP]:\t\t");
        else if(msg->priority == USER_P)
            printf("[USER]:\t\t");
        printf("%s\n", msg->text);
        msg=msg->next_mess;
    }
}

void printAll(){
    print_queue(vip_queue);
    print_queue(user_queue);
    if((vip_queue == NULL) && (user_queue == NULL) )
        printf("Both queues are empty\n");
}

void clear_queue(struct MESSAGE* vip_queue_, struct MESSAGE* user_queue_, int isAdministrator)
{
    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);
    init_queue();
  
    if(isAdministrator == ADMIN_P) 
    {
        printf("Deleted by administrator\n");

        struct MESSAGE* mess, *delete_mess;
        mess = vip_queue_;
        while(mess)
        {
            delete_mess = mess;
            mess = mess->next_mess;
            free(delete_mess->text);
            free(delete_mess);
        }

        mess = user_queue_;
        while(mess)
        {  
            delete_mess = mess;
            mess = mess->next_mess;
            free(delete_mess->text);
            free(delete_mess);
        }
    }
    
    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);
}

void clearAll()
{
    if( (vip_queue == NULL) && (user_queue == NULL) )
        printf("There is nothing to clear");
    else
        clear_queue(vip_queue, user_queue, ADMIN_P);   
}

void send_mess(struct MESSAGE* mess)
{
    if(mess->priority == VIP_P)
    {
        if(last_vip == NULL)
        {
            vip_queue = mess;
            last_vip = mess;
        }
        else
        {
            last_vip->next_mess = mess;
            last_vip = mess;
        }
    }
    else if(mess->priority == USER_P)
    {
        if(last_user == NULL)
        {
            last_user = mess;
            user_queue = mess;
        }
        else
        {
            last_user->next_mess = mess;
            last_user = mess;
        } 
    }   
}

void recieve_mess(struct MESSAGE* mess)
{
    if(vip_queue)
    {
        vip_queue = vip_queue->next_mess;

        if(mess == last_vip)
        {
            last_vip = NULL;
            vip_queue = NULL;
        }
        free(mess->text);
        free(mess);
    }
    else
    {
        if(user_queue)
        {
            user_queue = user_queue->next_mess;

            if(mess == last_user)
            {
                last_user = NULL;
                user_queue = NULL;
            }
            free(mess->text);
            free(mess);
        }
    }
    
}

void* chat()
{
    struct MESSAGE* mess;

    while(!sem_trywait(&full))
    {
        pthread_mutex_lock(&vip_queue_mutex);
        pthread_mutex_lock(&user_queue_mutex);

        if(vip_queue)
        {
            print_queue(vip_queue);
            mess = vip_queue;
            recieve_mess(mess);
        }
        else if(user_queue)
        {
            print_queue(user_queue);
            mess = user_queue;
            recieve_mess(mess);
        }

        pthread_mutex_unlock(&vip_queue_mutex);
        pthread_mutex_unlock(&user_queue_mutex);
        sem_post(&empty);

        sleep(1);
    }

    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);

    printAll();

    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);
    pthread_exit(NULL);
}

void* user(void* arg)
{
    struct MESSAGE* new_mess = malloc(sizeof(struct MESSAGE));
    int priority = *((int *) arg);
    char* priority_name[] = { "USER", "VIP", "ADMINISTRATOR"};
    {
        sem_wait(&empty);
        pthread_mutex_lock(&vip_queue_mutex);
        pthread_mutex_lock(&user_queue_mutex);

        char* text = malloc(20*sizeof(char));

        new_mess =(struct MESSAGE*)malloc(sizeof(struct MESSAGE));
        new_mess->next_mess = NULL;
        new_mess->priority = priority;

        sprintf(text, "Message number: %d", ++mess_num);
        new_mess->text = text;
        send_mess(new_mess);

        pthread_mutex_unlock(&user_queue_mutex);
        pthread_mutex_unlock(&vip_queue_mutex);
        sem_post(&full);
    }
    pthread_exit(NULL);
}



// Patrzymy czy da się odczytać z pustej kolejki
void test_empty()
{
    printf("\n\t\t ***TEST 1: EMPTY***\n");
    sleep(1);
    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);
    printAll();
    if( (vip_queue == NULL ) && (user_queue == NULL ) )
        printf("\n\t\tSUCCESS\n");
    else
        printf("\n\t\tFAIL\n");

    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);
    clear_queue(vip_queue, user_queue, ADMIN_P);
    mess_num = 0;

}

// Patrztmy czy możemy umieścić 4 userów - maksymalną liczbę 
void test_full()
{
    printf("\n\n\t\t***TEST 2: FULL***\n");
    pthread_t users[MAX_QUEUE];
    int vip_priority = VIP_P;
    int user_priority = USER_P;
    pthread_create(&users[0], NULL, &user, &user_priority);
    pthread_create(&users[1], NULL, &user, &user_priority);
    pthread_create(&users[2], NULL, &user, &vip_priority);
    pthread_create(&users[3], NULL, &user, &vip_priority);
    sleep(1);
    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);
    printAll();
    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);

    for(int i = 0; i < 4; ++i)
        pthread_cancel(users[i]);
    clearAll();
    printAll();
    mess_num = 0;
    printf("\n\t\tSUCCESS\n");
}

// Sprawdzamy czy VIP będzie zawsze na górze
void test_priority1()
{
    printf("\n\n\t\t***TEST 3: VIP PRIORYTY***\n");
    pthread_t users[MAX_QUEUE];
    int vip_priority = VIP_P;
    int user_priority = USER_P;
    pthread_create(&users[0], NULL, &user, &user_priority);
    pthread_create(&users[1], NULL, &user, &user_priority);
    pthread_create(&users[2], NULL, &user, &vip_priority);
    sleep(1);
    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);
    printAll();
    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);

    for(int i = 0; i < 3; ++i)
        pthread_cancel(users[i]);
    clearAll();

    mess_num = 0;
    printf("\n\t\tSUCCESS\n");
}

// Sprawdzamy czy USER będzie zawsze na dole
void test_priority2()
{
    printf("\n\n\t\t***TEST 4: USER PRIORYTY***\n");
    pthread_t users[MAX_QUEUE];
    int vip_priority = VIP_P;
    int user_priority = USER_P;
    pthread_create(&users[0], NULL, &user, &vip_priority);
    pthread_create(&users[1], NULL, &user, &user_priority);
    pthread_create(&users[2], NULL, &user, &user_priority);
    pthread_create(&users[3], NULL, &user, &vip_priority);
    sleep(1);
    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);
    printAll();
    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);
    
    clearAll();
    for(int i = 0; i < 4; ++i)
        pthread_cancel(users[i]);

    mess_num = 0;
    printf("\n\t\tSUCCESS\n");
}

// Sprawdzamy czyszczenie kolejek 
void test_clear()
{
    printf("\n\n\t\t***TEST 5: CLEAR***\n");
    pthread_t users[MAX_QUEUE];
    int vip_priority = VIP_P;
    int user_priority = USER_P;
    pthread_create(&users[0], NULL, &user, &user_priority);
    pthread_create(&users[1], NULL, &user, &user_priority);
    pthread_create(&users[2], NULL, &user, &vip_priority);
    pthread_create(&users[3], NULL, &user, &user_priority);
    sleep(1);
    pthread_mutex_lock(&vip_queue_mutex);
    pthread_mutex_lock(&user_queue_mutex);
    printAll();
    pthread_mutex_unlock(&vip_queue_mutex);
    pthread_mutex_unlock(&user_queue_mutex);

    for(int i = 0; i < 4; ++i)
        pthread_cancel(users[i]);
    clearAll();
    printAll();
    if((user_queue == NULL) && (vip_queue == NULL))
        printf("\n\t\tSUCCESS\n");
    mess_num = 0;
}

void test()
{
    test_empty();
    test_full();
    test_priority1();
    test_priority2();
    test_clear();
}

int main()
{
    test();
    return 0;
}
