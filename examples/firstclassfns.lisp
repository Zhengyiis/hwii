(define (f g) (g 2))
(define (mul2 x) (+ x x))

(define (add-one x) (+ x 1))
(define (add-two x) (+ x 2))

(define (range lo hi)
  (if (< lo hi)
   (pair lo (range (add1 lo) hi))
   false))
(define (map f l)
  (if (not l) l
  (pair (f (left l)) (map f (right l)))))

(define (g x) (+ x 1))

(do 
  (print (f mul2))
  (newline)
  (print ((left (pair add-one add-two)) 5))
  (newline)
  (let ((newname mul2)) 
   (print (newname 5)))
  (newline)
  (print (map (lambda (x) (+ x 1)) (range 0 4))))