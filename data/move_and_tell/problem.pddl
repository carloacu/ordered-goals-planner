(define
    (problem move_and_tell)
    (:domain move_and_tell)

    (:objects
        text_1 text_2 - text
        user_1 user_2 - user
        phone_2 - phone
    )

    (:init
        (is_detected user_1)
        (= (attention) user_1)
        (= (phone_of phone_2) user_2)
    )

    (:goal
        (and ;; __ORDERED
            (exists (?u - user) (sent text_1 ?u))
            (exists (?u - user) (sent text_2 ?u))
        )
    )

)
