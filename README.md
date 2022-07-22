# CommandPattern

Wybrałem prostą implementacje wzorca Polecenie. Klasy, które są poleceniami dziedziczą po CommandBase i nadpisują funkcję wykonującą. 
Klasa SellOrder posiada polecenie sprzedania jakiejś rzeczy, BuyOrder kupna. Klasa Pawnshop przyjmuje zamówienia i dodaje je do kolejki poleceń.
Przyjęcie zamówień powinno być odbywać się w wątku w którym została stworzona kolejka zamówień, a wykonanie poleceń może być zrobione na drugim wątku.
