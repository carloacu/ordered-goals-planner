(define (domain millionaire)

  (:requirements
    :typing
    :numeric-fluents
    :negative-preconditions
  )

  (:types agent)

  (:predicates
    (has-company    ?a - agent)
    (joined-startup ?a - agent)
    (joined-megacorp ?a - agent)
    (has-licence    ?a - agent)
    (has-master     ?a - agent)
    (week-done      ?a - agent)
  )

  (:functions
    (money  ?a - agent)
    (health ?a - agent)
    (hours  ?a - agent)
  )

  (:action sleep
    :parameters (?a - agent)
    :precondition (< (health ?a) 80)
    :effect (increase (health ?a) 20)
  )

  (:action vacation
    :parameters (?a - agent)
    :precondition (and
      (has-company ?a)
      (>= (money ?a) 10000)
      (>= (hours ?a) 120)
    )
    :effect (and
      (decrease (money  ?a) 10000)
      (increase (health ?a) 60)
      (assign   (hours  ?a) 0)
      (not (week-done ?a))
    )
  )

  (:action join-startup
    :parameters (?a - agent)
    :precondition (and
      (not (has-company ?a))
      (has-licence ?a)
    )
    :effect (and
      (has-company    ?a)
      (joined-startup ?a)
    )
  )

  (:action join-megacorp
    :parameters (?a - agent)
    :precondition (and
      (not (has-company ?a))
      (has-master ?a)
    )
    :effect (and
      (has-company     ?a)
      (joined-megacorp ?a)
    )
  )

  (:action work-startup
    :parameters (?a - agent)
    :precondition (and
      (joined-startup ?a)
      (>= (health ?a) 50)           ; healthCostWork(40) + 10
    )
    :effect (and
      (increase (money  ?a) 40000)
      (decrease (health ?a) 40)
      (increase (hours  ?a) 40)
      (when (>= (hours ?a) 40) (week-done ?a))
    )
  )

  (:action work-megacorp
    :parameters (?a - agent)
    :precondition (and
      (joined-megacorp ?a)
      (>= (health ?a) 50)           ; healthCostWork(40) + 10
    )
    :effect (and
      (increase (money  ?a) 60000)
      (decrease (health ?a) 40)
      (increase (hours  ?a) 40)
      (when (>= (hours ?a) 40) (week-done ?a))
    )
  )

  (:action overtime-startup
    :parameters (?a - agent)
    :precondition (and
      (joined-startup ?a)
      (week-done ?a)
      (>= (health ?a) 55)           ; healthCostOT(45) + 10
    )
    :effect (and
      (increase (money  ?a) 80000)
      (decrease (health ?a) 45)
      (increase (hours  ?a) 20)
    )
  )

  (:action overtime-megacorp
    :parameters (?a - agent)
    :precondition (and
      (joined-megacorp ?a)
      (week-done ?a)
      (>= (health ?a) 60)           ; healthCostOT(50) + 10
    )
    :effect (and
      (increase (money  ?a) 120000)
      (decrease (health ?a) 50)
      (increase (hours  ?a) 20)
    )
  )

  (:action study-university
    :parameters (?a - agent)
    :precondition (and
      (not (has-licence ?a))
      (>= (money  ?a) 5000)
      (>= (health ?a) 35)           ; healthCost(25) + 10
    )
    :effect (and
      (has-licence      ?a)
      (decrease (money  ?a) 5000)
      (decrease (health ?a) 25)
    )
  )

  (:action study-highschool
    :parameters (?a - agent)
    :precondition (and
      (not (has-master ?a))
      (>= (money  ?a) 20000)
      (>= (health ?a) 45)           ; healthCost(35) + 10
    )
    :effect (and
      (has-master       ?a)
      (decrease (money  ?a) 20000)
      (decrease (health ?a) 35)
    )
  )
)