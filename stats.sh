#!/bin/bash
if [ ! -f "data.txt" ]; then
    echo "Le fichier data.txt n'existe pas."
    exit 1
fi


cat << EOF > grm.gp
set terminal pdfcairo font "Times New Roman,12" size 20cm,20cm;
set output "statistiques.pdf"
set autoscale fix
unset key
set xlabel "Temps de reaction en secondes"
set ylabel "Nombre de manches"
set bmargin at screen 0.2
set tmargin at screen 0.9
set border 3
set xrange [0:*]
set yrange [0:*]
set xtics scale 0  
set ytics scale 0   
set xtics 0.5       
set style line 2 lc rgb "red" pt 7 ps 0.5  
plot "data.txt" using 1:3 with points linestyle 2
EOF

gnuplot grm.gp

xdg-open statistiques.pdf

rm grm.gp

echo "Le PDF a été généré avec succés."