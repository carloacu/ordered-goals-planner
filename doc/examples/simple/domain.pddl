(define (domain move-pickup)
  (:requirements :strips :typing)

  (:types
    robot location object
  )

  (:predicates
    (at ?robot - robot ?location - location)
    (at-object ?object - object ?location - location)
    (holding ?robot - robot ?object - object)
  )

  (:action move
    :parameters (?robot - robot ?from - location ?to - location)
    :precondition (at ?robot ?from)
    :effect (and (not (at ?robot ?from))
                 (at ?robot ?to))
  )

  (:action pick-up
    :parameters (?robot - robot ?object - object ?location - location)
    :precondition (and (at ?robot ?location)
                       (at-object ?object ?location))
    :effect (and (not (at-object ?object ?location))
                 (holding ?robot ?object))
  )

  (:action drop
    :parameters (?robot - robot ?object - object ?location - location)
    :precondition (and (at ?robot ?location)
                       (holding ?robot ?object))
    :effect (and (at-object ?object ?location)
                 (not (holding ?robot ?object)))
  )
)
