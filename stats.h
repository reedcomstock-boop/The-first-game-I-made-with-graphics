#ifndef STATS_H
#define STATS_H

struct Stats {
    double strength;
    double dexterity;
    double intelligence;
    double defence;



    Stats operator+(const Stats& other) const {
        return Stats{
            strength + other.strength,
            dexterity + other.dexterity,
            intelligence + other.intelligence,
            defence + other.defence,

        };
    }

    Stats& operator+=(const Stats& other) {
        strength += other.strength;
        dexterity += other.dexterity;
        intelligence += other.intelligence;
        defence += other.defence;

        return *this;
    }
};

#endif // STATS_H