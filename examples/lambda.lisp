(define (map f l)
  (if (not l) l
  (pair (f (left l)) (map f (right l)))))



(define (f g) (g 2))
(define (mul2 x) (+ x x))
(print (f (lambda (x) (+ x x))))