(define
    (domain move_and_tell)
    (:requirements :strips :typing)

    (:types
        phone
        text
        user
    )

    (:predicates
        (is_detected ?po - user)
        (sent ?text - text ?u - user)
    )

    (:functions
        (attention) - user
        (phone_of ?p - phone) - user
    )


    (:durative-action look_at
        :parameters
            (?u - user)

        :duration (= ?duration 1)

        :condition
            (at start (is_detected ?u))

        :effect
            (at end (assign (attention) ?u))
    )



    (:durative-action search
        :parameters
            (?p - phone)

        :duration (= ?duration 1)

        :effect
            (at end (forall (?u - user) (when (= (phone_of ?p) ?u) (is_detected ?u))))
    )



    (:durative-action send
        :parameters
            (?receiver - user ?something - text)

        :duration (= ?duration 1)

        :condition
            (at start (= (attention) ?receiver))

        :effect
            (at end (sent ?something ?receiver))
    )

)
