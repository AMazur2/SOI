#include <iostream>
#include "monitor.h"
#include <thread>

#define VIP_P 1
#define USER_P 0

#define MAX_MESS 64
#define MIN_MESS 8

const int MAX = 4; //maksymalny rozmiar kolejki

struct MESSAGE
{
    std::string text;
    int priority;
    MESSAGE* next_mess;
};

class MyMonitor : Monitor
{
    public:
        unsigned int queue_size;                                //obecny stan bufora
        MESSAGE *queue;                                         //wskaźnika na początek kolejki
        MESSAGE *last_vip;                                      //wskaźnik na ostatniego VIPa
        MESSAGE *last_user;                                     //wskaźnika na ustatniego usera                           
        Condition full, empty;

        MyMonitor()                                             //konstruktor
        {
            queue = nullptr;
            last_vip = nullptr;
            last_user = nullptr;
            queue_size = 0;
        }

        void consumer(void)                                     //konsument
        {
            enter();                                            //wchodzimy do monitora
            if(queue_size == 0)
            {                                                   //jeśli kolejka pusta to czekaj
                std::cout<<"I'm waiting"<<std::endl;
                wait(empty);
                std::cout<<"Something has been added"<<std::endl;
            }
            show_queue();                                       
            recieve(queue);                                     //po wyświetleniu, usuń pierwszy element
            --queue_size;                                       //zmiejsz licznik wiadomości
            if(queue_size == MAX - 1)
                signal(full);
            leave();
        }

        void producer(struct MESSAGE* mess)                     //producent
        {
            enter();
            if(queue_size == MAX )
                wait(full);
            send_mess(mess);                                    //wysyłamy wiadomość
            ++queue_size;                                       //zwiększamy licznik wiadomości
            if(queue_size == 1)
                signal(empty);
            leave();
        }
        
        void show_queue()                                       //pokaż kolejkę
        {
            if(queue != nullptr)
            {
                struct MESSAGE* m = queue;
                while(m)
                {
                    std::cout<<"[ "<<m->priority<<" ] : "<<m->text<<std::endl;
                    m=m->next_mess;
                }
            }
            else
                std::cout<<"Queue is empty!"<<std::endl;
        }

        void recieve(struct MESSAGE* mess)                      //usuń pierwszę fiadomość
        {
            if(queue)                                           //jeżeli jest coś w tej wiadomości 
            {
                queue = queue->next_mess;
                if(mess == last_vip)
                    last_vip = nullptr;
                if(mess = last_user)
                {
                    last_user = nullptr;
                    queue = nullptr;
                }
                delete mess;                                    //usuń wiadomość
            }
        }

        void send_mess(struct MESSAGE* m)                       //wysyłanie wiadomości do kolejki
        {
            if(m->text.size()<MIN_MESS)                                //sprawdzamy rozmiar
                m->text = "MESSAGE WAS TOO SHORT";              //jak za mały to zamieniamy na informacje
            if(m->text.size()>MAX_MESS)                               //jak za duży to zamienamy ze za długa wiadomość
                m->text = "MESSAGE WAS TOO LONG: I COULDN'T SEND IT!";
            
            if(queue == nullptr)                                //dodaj w odpowiednie miejsce
            {
                queue = m;
                if(m->priority == VIP_P)
                    last_vip = m;
                if(m->priority == USER_P)
                    last_user = m;
            }
            else if( m->priority == VIP_P )
            {
                if(last_vip == nullptr)
                {
                    m->next_mess=queue;
                    queue = m;
                }
                else
                {
                    m->next_mess = last_vip->next_mess;
                    last_vip->next_mess = m;
                }
                last_vip = m;
            }
            else
            {
                if(last_user == nullptr)
                    last_vip->next_mess = m;
                else
                    last_user->next_mess = m;

                last_user = m;
            }
        }

        void clear()                                            //czyszczenie kolejki
        {
            enter();
            struct MESSAGE* m, *delete_m;
            m = queue;
            while(m)
            {
                delete_m = m;
                m = m->next_mess;
                delete delete_m;                                //usuń wszystkie wiadomości
            }
            leave();
            queue = last_user = last_vip = nullptr;             //ustaw wskaźniki na nullptr
            queue_size = 0;                                     //ustaw licznik wiadomości na 0
        }
};

MyMonitor monit;

void create_producer(std::string text, int priority)            //stworzenie producenta
{
    struct MESSAGE* new_mess;
    try
    {
        new_mess = new MESSAGE;
    }
    catch(const std::bad_alloc& ba)
    {
        std::cerr << "bad_alloc caught: " << ba.what() << '\n';
    }

    new_mess->next_mess == nullptr;
    new_mess->priority = priority;
    new_mess->text = text;
    if(priority == VIP_P)
        std::cout<<"\nVIP is trting to send message: " << text <<std::endl;
    if(priority == USER_P)
        std::cout<<"\nUSER is trying to send message: " << text <<std::endl;
    monit.producer(new_mess);
}

int letter_num()                                                //losowanie długości słowa
{
    srand( time (NULL) );
    int k = std::rand()%80;
    return k;
}

std::string create_text(int length)                             //losowanie słowa
{
    srand(time(NULL));
    std::string letters = "abcdefghijklmnopqrestuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    char mess[length];
    for( int i = 0; i < length; ++i)
        mess[i] = letters[std::rand()%(letters.size()-1)];

    std::string text;
    text = mess;
    return text;
}

void reader( int times )                                        //stworzenie "czytelnika"
{
    for(int i = 0; i < times; ++i)
    {
        monit.consumer();
        sleep(1);
    }
}

void test1()                                                    //testowanie priorytetu VIP'a
{

    std::cout<<"\t\t***TEST 1***"<<std::endl;
    int l1 =letter_num();
    sleep(1);
    int l2 =letter_num();
    sleep(1);
    int l3 =letter_num();
    sleep(1);
    std::string m1 = create_text(l1);
    sleep(1);
    std::string m2 = create_text(l2);
    sleep(1);
    std::string m3 = create_text(l3);
    sleep(1);
    std::thread p1(create_producer, m1 , USER_P);
    std::thread p2(create_producer, m2 , USER_P);
    std::thread p3(create_producer, m3 , VIP_P);
    sleep(1);
    std::cout<<std::endl;
    monit.show_queue();
    p1.join();
    p2.join();
    p3.join();
    monit.clear();
    std::cout<<std::endl<<std::endl<<std::endl;
}

void test2()                                                    //testowanie priorytetu USER'a
{
    std::cout<<"\t\t***TEST 2***"<<std::endl;
    std::thread p1(create_producer,"Vip nr 1!", VIP_P);
    std::thread p2(create_producer, "User nr 1", USER_P);
    std::thread p3(create_producer, "VIP nr 2!", VIP_P);
    sleep(1);
    std::cout<<std::endl;
    monit.show_queue();
    p1.join();
    p2.join();
    p3.join();
    monit.clear();
    std::cout<<std::endl<<std::endl<<std::endl;
}

void test3()                                                    //pełna kolejka
{
    std::cout<<"\t\t***TEST 3***"<<std::endl;
    std::thread p1(create_producer,"Vip nr 1!", VIP_P);
    std::thread p2(create_producer, "User nr 1", USER_P);
    std::thread p3(create_producer, "VIP nr 2!", VIP_P);
    std::thread p4(create_producer, "USER nr 2", USER_P);
    std::thread p5(create_producer, "VIP nr 3!", VIP_P);
    std::thread c(reader, 1);
    sleep(2);
    std::cout<<std::endl;
    monit.show_queue();
    p1.join();
    p2.join();
    p3.join();
    p4.join();
    p5.join();
    sleep(3);
    c.join();
    monit.clear();
    std::cout<<std::endl<<std::endl<<std::endl;
}

void test4()                                                    //pusta kolejka
{
    std::cout<<"\t\t***TEST 4***"<<std::endl;
    std::cout<<"Testing buf_size:"<<std::endl;
    std::cout<<"Actual buf_size: "<<monit.queue_size << std::endl;
    std::thread r(reader, 1);
    sleep(3);
    std::thread p(create_producer, "I'm here actually", USER_P);
    p.join();
    r.join();
    std::cout<<std::endl<<std::endl<<std::endl;
}

void test5()                                                    //krótka wiadomość
{
    std::cout<<"\t\t***TEST 5***"<<std::endl;
    std::string m = "aaa";
    std::cout<<"Trying to send message: \"" << m << "\". Size of this message: " << m.size() << std::endl;
    std::thread p(create_producer, m, USER_P);
    sleep(1);
    std::cout<<std::endl;
    monit.show_queue();
    p.join();
    monit.clear();
    std::cout<<std::endl<<std::endl<<std::endl;
}

void test6()                                                    //długa wiadomość
{
    std::cout<<"\t\t***TEST 6***"<<std::endl;
    std::string m = "I want to be a very very long message. I want to be longer than I can!!!";
    std::cout<<"Trying to send message: \"" << m << "\". Size of this message: " << m.size() << std::endl;
    std::thread p(create_producer, m , VIP_P);
    sleep(1);
    std::cout<<std::endl;
    monit.show_queue();
    p.join();
    monit.clear();
    std::cout<<std::endl<<std::endl<<std::endl;
}

int main()
{
    test1();
    test2();
    test3();
    test4();
    test5();
    test6();

    return 0;
}
