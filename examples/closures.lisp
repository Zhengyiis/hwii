(define (f g) (g 2))
(let ((y 3)) (print (f (lambda (x) (+ x y)))))