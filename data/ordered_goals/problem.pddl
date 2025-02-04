(define (problem move-pickup-problem)
  (:domain move-pickup)

  (:objects
    robot1 - robot
    locationA locationB locationC - location
    box1 - object
  )

  (:init
    (at robot1 locationA)
    (at-object box1 locationB)
  )

  (:ordered-goals
    :goals
      (ordered-list
        (started_notified)
        (and
          (at robot1 locationA)
          (at-object box1 locationC)
        )
        (finished_notified)
      )
  )
)
