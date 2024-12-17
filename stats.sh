#!/bin/bash
if [ ! -f "donnees.txt" ]; then
    echo "Le fichier donnees.txt n'existe pas."
    exit 1
fi

awk '
{
    for (i = 1; i <= 100; i++) 
    {
        if ($2 == i) 
        {
            count[i]++
        }
    }
} 
END {
    for (i = 1; i <= 100; i++) 
    {
        if (count[i] > 0) 
        {
            print i, count[i] > "comptage.txt"
        }
    }
}' donnees.txt

cat << EOF > graph_manche.gp
set terminal png font "Times,12" size 2000,800
set output "graph_manche.png"
set title "Temps de réaction moyen en fonction de de la dernière manche jouée."
set style line 2 lc rgb "red" pt 7 ps 0.75
set border 3
unset key
set xrange [0:*]
set yrange [0:12]
set xtics scale 0
set ytics scale 0
set xlabel "Temps de réaction en secondes"
set ylabel "Manche"
set xtics 0.5
plot "donnees.txt" using 1:3 with points linestyle 2
EOF

gnuplot graph_manche.gp

cat << EOF > graph_occur.gp
set terminal png font "Times,12" size 2000,800
set output "graph_occur.png"
set title "Nombre d'occurences de la dernière carte jouée."
set xlabel "Nombres"
set ylabel "Occurrences"
set border 3
unset key
set style histogram gap 1.5 
set style line 1 lc rgb "green" pt 7 ps 0.5
set style fill solid 1.0 border -1
set xtics scale 0
set ytics scale 0
set ytics 1
set xtics 1
set xrange [1:100]
set yrange [0:*]
set boxwidth 0.25
plot "comptage.txt" using 1:2 with boxes linestyle 1
EOF

gnuplot graph_occur.gp

cat << EOF > stats.tex
\documentclass[10pt,a4paper]{article}
\usepackage[a4paper, top=2.5cm, bottom=2cm, left=2cm, right=2cm, headheight=14pt]{geometry} 
\usepackage{graphicx}           
\usepackage{pdflscape}  
\usepackage{fancyhdr}
\pagestyle{plain}    
\begin{document}
\title{Statistiques des données recueillies}
\author{}
\maketitle
\newgeometry{top=0cm, bottom=0cm, left=0cm, right=0cm, headheight=0pt}  
\thispagestyle{empty}
\begin{landscape}
\vfill
\begin{figure}
    \centering
    \includegraphics[scale=0.5]{graph_manche.png}
\end{figure}
\vfill
\newpage
\thispagestyle{empty}
\vfill
\begin{figure}
    \centering
    \includegraphics[scale=0.5]{graph_occur.png}
\end{figure}
\vfill
\end{landscape}
\restoregeometry
\end{document}
EOF

pdflatex stats.tex

rm graph_manche.gp graph_occur.gp stats.log stats.aux comptage.txt graph_manche.png graph_occur.png

echo "Le PDF a été généré avec succès"