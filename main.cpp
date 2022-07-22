#include <iostream>
#include <vector>
#include <array>
#include <random>

#include "CommandQueue.h"

struct BuyOrder : CommandBase
{
    explicit BuyOrder(std::string item)
        : CommandBase(Execute), item(std::move(item))
    {
    }

    static inline void Execute(CommandBase* base, uintptr_t& next)
    {
        next = align(sizeof(BuyOrder));
        auto self = dynamic_cast<BuyOrder*>(base);

        std::cout << "Kupuje " << self->item <<"\n";

        self->~BuyOrder();
    }

private:
    std::string item;
};

struct SellOrder : CommandBase
{
    explicit SellOrder(std::string item)
        : CommandBase(Execute), item(std::move(item))
    {
    }

    static inline void Execute(CommandBase* base, uintptr_t& next)
    {
        next = align(sizeof(SellOrder));
        auto self = dynamic_cast<SellOrder*>(base);

        std::cout << "Sprzedaje " << self->item << "\n";

        self->~SellOrder();
    }

private:
    std::string item;
};

struct Pawnshop
{
    explicit Pawnshop(CommandQueue& queue)
        : queue(queue)
    {
    }

    template<typename Command, typename ... Args>
    void TakeOrder(Args&& ... args) requires(std::is_base_of_v<CommandBase, Command>)
    {
        queue.AllocateCommand<Command>(std::forward<Args>(args)...);
    }

    CommandQueue& queue;
};

#define SINGLE_THREAD

int main()
{
#ifdef SINGLE_THREAD
    //jednowątkowy przykład
    {
        CommandQueue commandQueue(false);

        Pawnshop pawnshop(commandQueue);

        pawnshop.TakeOrder<BuyOrder>("IPhone XS");
        pawnshop.TakeOrder<SellOrder>("Lenovo L15");

        commandQueue.Flush();
        bool executeResult = commandQueue.Execute();

        commandQueue.Quit();
    }
#endif

#ifndef SINGLE_THREAD
    // komendy mogą być też wykonywane na drugim wątku,
    // ale muszą być dodane z wątka głównego (można łatwo zmienić żeby inne wątki mogły też dodawać komendy
    {
        constexpr std::array<const char*, 15> things = {
                "Samsung Galaxy A20",
                "Samsung Galaxy note 10",
                "Monitor Benq XL2411",
                "Klawiatura Razer BlackWidow V3",
                "Myszka SteelSeries Sensei Ten",
                "Glosniki Logitech Z533",
                "Telewizor LG",
                "Podkladka Steelseries QCK Plus",
                "Karta Graficzna RTX2080",
                "Procesor Intel Core i5 7600k",
                "Plyta glowna MSI Tomahawk B450",
                "Zegarek Rolex",
                "Ekspres do kawy",
                "Pralka Bosh",
                "Zmywarka Electrolux"
        };

        CommandQueue commandQueue;

        std::jthread commandThread([&commandQueue](const std::stop_token& stopToken){
            while(commandQueue.Execute()) {
            }
        });

        Pawnshop pawnshop(commandQueue);

        std::random_device rndGen;
        for(auto item : things) {
            const std::uniform_int_distribution<uint16_t> distribution(0, static_cast<uint16_t>(things.size()) - 1);
            const uint16_t index = distribution(rndGen);

            if(index % 2 == 0) {
                pawnshop.TakeOrder<BuyOrder>(item);
            } else {
                pawnshop.TakeOrder<SellOrder>(item);
            }
        }

        //przy takiej małej ilości klatek lub/i ilości rzeczy w tablicy "things" może wydawać się,
        //wszystko jest wykonywane synchronicznie
        uint32_t frameCount = 10;
        while(frameCount-- > 0) {
            commandQueue.Flush();
            std::cout << "Robie cos sobie dodatkowo\n";
        }

        commandQueue.Quit();
    }
#endif

    return 0;
}
