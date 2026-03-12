(define
    (problem harmony)
    (:domain harmony)
    (:requirements :ordered-goals)

    (:objects
        location1 - location
    )

    (:init
        (= (focused me) unknown_human)
        (= (is_carrying_too_heavy_payload) false)
        (is_detected unknown_human)
        (= (location_of me) location1)
        (= (location_of unknown_human) location1)
        (= (wanted_location_of unknown_human) location1)
    )

    (:ordered-goals
        :effect-between-goals
            (forall (?resource - entity) (not (resource_has_been_taken ?resource)))

        :goals
            (ordered-list
                (forall (?user_param - user) (imply
                    (and
                        (not (= (wanted_location_of ?user_param) undefined))
                        (= (location_of ?user_param) (wanted_location_of ?user_param))
                    )
                    (= (focused me) undefined)
                ))
            )
    )

)
