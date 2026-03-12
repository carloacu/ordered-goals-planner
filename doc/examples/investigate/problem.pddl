(define
    (problem harmony)
    (:domain harmony)
    (:requirements :ordered-goals)

    (:objects
        location1 - location
    )

    (:init
        (= (focused me) unknown_human)
        (= (location_of unknown_human) location1)
        (= (wanted_location_of unknown_human) location1)
    )

    (:ordered-goals
        :goals
            (ordered-list
                (forall (?user_param - user) (imply
                    (not (= (wanted_location_of ?user_param) undefined))
                    (and
                        (= (location_of ?user_param) (wanted_location_of ?user_param))
                        (not (= (focused me) ?user_param))
                    )
                ))
            )
    )

)
