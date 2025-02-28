#!/usr/bin/python

# Plot given data (2 column) with given axes, optional logscale
# and range.

import sys
import getopt

# Usage/help message display
def usage():
    print(sys.argv[0], " [options] data-file-name legend-key fields [data-file-name legend-key fields ...]")
    print("The available options are:")
    print("-x         X-axis label within quotes")
    print("-y         Y-axis label within quotes")
    print("--lx       Use log scale on X-axis")
    print("--ly       Use log scale on Y-axis")
    print("-t         Title of the graph in quotes")
    print("--vl       Vertical line x-values (comma-separated)")
    print("-f         Terminal format")
    print("-o         Output filename")
    print("--xrange   x1:x2 within quotes")
    print("--yrange   y1:y2 within quotes")
    print("--bmargin  Bottom margin (in what units?)")
    print("--rmargin  Right  margin (in what units?)")
    print("--lmargin  Left   margin (in what units?)")
    print("--small    Use small plot configuration for axes")
    print("--gopt     Options for 'set grid'")
    print("-l         Plot using lines (default is linespoints)")
    print("-b         Plot using boxes")
    print("-i         Plot using points")
    print("-e         Plot with yerrorbars")
    print("-k         Key (legend) parameters")
    print("-g         Set grid on")
    print("-p         Point size")
    print("-a         Tics")
    print("--lw       Default linewidth for curves in the graph")
    print("-h, --help Display this help message")

def main():
    # Get arguments
    args = sys.argv[1:]

    # get arguments
    try:
        options, real_args = getopt.gnu_getopt(args,
                                               "helbigx:y:t:f:o:k:p:a:", 
                                               ["lx", "ly", "xrange=",
                                                "yrange=", "bmargin=",
                                                "rmargin=", "lmargin=",
                                                "gopt=", "help",
                                                "small", "vl=",
                                                "lw="])
    except getopt.GetoptError as err:
        print(str(err))
        usage()
        sys.exit(2)

    # Is the data file argument provided?
    if len(real_args) == 0:
        usage()
        sys.exit(0)
        
    if len(real_args) % 3 != 0:
        usage()
        sys.exit(0)

    num_curves = int(len(real_args) / 3)
    retain_legend = True
    if num_curves >= 7:
        retain_legend = False

    lines = False # plotting using lines turned off by default. Usually, plot by linespoints.
    boxes = False # plotting using boxes, default is linespoints
    points = False # plotting using points
    bmargin_set = False # for checking bottom margin setting
    rmargin_set = False # for checking right margin setting
    lmargin_set = False # for checking left margin setting
    smallplot_set = False # for using small plot configurations
    def_lw = 5 # default linewidth for curves in the graph

    for opt, val in options:
        if opt in ("-h", "--help"):
            usage()
            sys.exit(0)
        elif opt == "-g":
            print("set grid")
        elif opt == "--small":
            smallplot_set = True
        elif opt == "-x":
            if smallplot_set == False:
                print("set xlabel " + "\"" + val + "\" offset 0,-1.5 font \"Times,30\"")
            else:
                print("set xlabel " + "\"" + val + "\" offset 0,-2 font \"Times,40\"")
        elif opt == "-y":
            if smallplot_set == False:
                print("set ylabel " + "\"" + val + "\" offset -2,0 font \"Times,30\"")
            else:
                print("set ylabel " + "\"" + val + "\" offset -5,0 font \"Times,40\"")
        elif opt == "-t":
            print("set title  " + "\"" + val + "\"")
        elif opt == "--lx":
            print("set logscale x")
        elif opt == "--ly":
            print("set logscale y")
        elif opt == "--xrange":
            print("set xrange " + val)
        elif opt == "--yrange":
            print("set yrange " + val)
        elif opt == "-l":
            lines = True
        elif opt == "-b":
            boxes = True
            # set some default box options
            print("set boxwidth 0.5")
            print("set style fill solid")
        elif opt == "-i":
            points = True
        elif opt == "-f":
            print("set terminal " + val)
        elif opt == "-o":
            print("set output " + "\"" + val + "\"")
        elif opt == "-k":
            if retain_legend:
                print("set key " + val)
        elif opt == "--bmargin":
            bmargin_set = True
            print("set bmargin " + val)
        elif opt == "--rmargin":
            rmargin_set = True
            print("set rmargin " + val)
        elif opt == "--lmargin":
            lmargin_set = True
            print("set lmargin " + val)
        elif opt == "-p":
            print("set pointsize " + val)
        elif opt == "-a":
            print("set tics " + val)
        elif opt == "--gopt":
            print("set grid " + val)
        elif opt == "--vl":
            xvals = val.split(',')
            for x in xvals:
                print(("set arrow from %s,graph(0,0) to %s,graph(1,1)"
                       % (x, x)))
        elif opt == "--lw":
            def_lw = int(val)

    if lmargin_set == False:
        print("set lmargin 12")
    if bmargin_set == False:
        print("set bmargin 4")
    if rmargin_set == False:
        print("set rmargin 3.5")
    print("set tmargin 2")
    
    print(("set style line 1 linewidth %d pt 4" % def_lw))
    print(("set style line 2 linecolor rgb \"black\" linewidth %d pt 6"
           % def_lw))
    print(("set style line 3 linewidth %d pt 8" % def_lw))
    print(("set style line 4 linewidth %d pt 5" % def_lw))
    print(("set style line 5 linewidth %d linecolor rgb \"brown\" pt"
           " 7" % def_lw))
    print(("set style line 6 linewidth %d linecolor rgb \"orange\" pt "
           "7" % def_lw)) 
    print(("set style line 7 linewidth %d linecolor rgb \"#11FF11\" pt "
           "7" % def_lw))
    print(("set style line 8 linewidth %d linecolor rgb \"violet\" pt "
           "7" % def_lw))
            
    outstr = "plot "
    
    nsets = int(len(real_args)/3)
            
    for i in range(0, nsets):

        filename, legend_key, fields = real_args[0:3]

        outstr += ("\"" + filename + "\" using " + fields + " with ")
        if lines:
            outstr += "lines"
        elif boxes:
            outstr += "boxes"
        elif points:
            outstr += "points"
        else:
            outstr += "linespoints"

        outstr += (" ls " + str(i+1))

        if retain_legend:
            outstr += (" title \"" + legend_key + "\"")
        else:
            outstr += (" title \"\" ")
        
        if i != (nsets - 1):
            outstr += ", "

        real_args = real_args[3:]

    print(outstr)

# Call main() if this is the main thread
if __name__ == "__main__":
    main()

