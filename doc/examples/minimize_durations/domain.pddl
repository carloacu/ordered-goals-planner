(define
    (domain harmony)
    (:requirements :strips :typing)

    (:types
        boolean number string location physical_object - entity
        physical_agent - physical_object
        user robot - physical_agent
    )

    (:constants
        false true - boolean
        me - robot
        unknown_human - user
    )

    (:predicates
        (holds ?r - robot ?o - physical_object)
        (is_detected ?po - physical_object)
        (resource_has_been_taken ?resource - entity)
        (resource_taken ?resource - entity)
    )

    (:functions
        (focused ?a - physical_agent) - physical_object
        (location_of ?o - physical_object) - location
        (wanted_location_of ?o - physical_object) - location
    )


    (:event move_holding_objects

        :parameters
            (?po - physical_object ?loc - location)

        :precondition
            (and
                (holds me ?po)
                (= (location_of me) ?loc)
            )

        :effect
            (assign (location_of ?po) ?loc)
    )

    (:durative-action move
        :parameters
            (?location - location)

        :duration (= ?duration 15)

        :effect
            (and
                (at end (assign (location_of me) ?location))
            )
    )

    (:durative-action move_slowly
        :parameters
            (?destination - location)

        :duration (= ?duration 20)

        :effect
            (and
                (at end (assign (location_of me) ?destination))
            )
    )

    (:durative-action un_focus
        :duration (= ?duration 10)

        :effect
            (at end (assign (focused me) undefined))
    )


)
