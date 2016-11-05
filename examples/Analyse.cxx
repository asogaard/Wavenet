// STL include(s).
#include <string> /* std::string */
#include <vector> /* std::vector */
#include <regex> /* std::regex */
#include <cstdio> /* snprintf */

// ROOT include(s).
#include "TStyle.h"
#include "TCanvas.h"
#include "TH2.h"
#include "TGraph.h"
#include "TMarker.h"
#include "TLine.h"
#include "TEllipse.h"
#include "TColor.h"
#include "TLatex.h"

// Wavenet include(s).
#include "Wavenet/Wavenet.h"
#include "Wavenet/Snapshot.h"
#include "Wavenet/Generators.h"

using namespace std;
using namespace arma;

int main (int argc, char* argv[]) {
    
    cout << "Running Wavenet analysis." << endl;

    //wavenet::GeneratorMode mode = wavenet::GeneratorMode::Gaussian;
    wavenet::GeneratorMode mode = wavenet::GeneratorMode::File;
    //wavenet::GeneratorMode mode = wavenet::GeneratorMode::Needle;
    const int M = 5;
    int Nfilter = 4;
    
    /* --- */
    
    wavenet::Wavenet wavenet;
    wavenet.doWavelet(true);
    
    // Variables.
    string outdir  = "./output/";
    string project = "Run.";
    switch (mode) {
        case wavenet::GeneratorMode::File:
            // ...
            project += "File";
            break;
            
        case wavenet::GeneratorMode::Uniform:
            project += "Uniform";
            break;
            
        case wavenet::GeneratorMode::Needle:
            project += "Needle";
            break;
            
        case wavenet::GeneratorMode::Gaussian:
            project += "Gaussian";
            break;
            
        default:
            cout << "Event mode not recognised." << endl;
            return 0;
            break;
    }
    project += ".N" + to_string(Nfilter);
    
    outdir += project + "/";
    
    string pattern = outdir + "snapshots/" +  project + ".%06u.snap";
    
    wavenet::Snapshot snap (pattern);
    
    vector< TGraph > costGraphs (M);
    vector< TGraph > filterGraphs (M);
    TCanvas c ("c", "", 700, 600);
    
    wavenet.save(outdir + "tmp.snap");
    
    wavenet::GeneratorBase* generator = nullptr;
    if (mode == wavenet::GeneratorMode::File) {
        //generator = new wavenet::HepMCGenerator("input/Pythia.WpT500._000001.hepmc");
        if (!generator || !generator->good()) {
            return 1;
        }
    } else if (mode == wavenet::GeneratorMode::Needle) {
        generator = new wavenet::NeedleGenerator();
    } else if (mode == wavenet::GeneratorMode::Uniform) {
        generator = new wavenet::UniformGenerator();
    } else if (mode == wavenet::GeneratorMode::Gaussian) {
        generator = new wavenet::GaussianGenerator();
    }

    generator->setShape({16, 16});
    
    
    vector< Mat<double> > examples;
    for (unsigned i = 0; i < 10; i++) {
        examples.push_back( generator->next() );
        std::unique_ptr<TH1> exampleSignal = wavenet::MatrixToHist(examples.at(i), 3.2);
        exampleSignal->Draw("COL Z");
        c.SaveAs((outdir + "exampleSignal." + to_string(i + 1) +"." + project + ".pdf").c_str());
    }
    generator->close();


    // * Run    
    double minCost = 99999., maxCost = 0;
    
    int bestBasis = 0;
    
    unsigned longestCost = 0;
    while (snap.exists() && snap.number() <= M) {
        
        wavenet.load(snap.file());
        wavenet.print();
        
        auto filterLog = wavenet.filterLog();
        auto costLog   = wavenet.costLog();
        
        //costLog.pop_back();
        
        if (costLog.size() > longestCost) {
            longestCost = costLog.size();
        }
        const unsigned Ncoeffs = filterLog.size();
        
        // Cost graphs.
        costGraphs.at(snap.number() - 1) = wavenet::costGraph( costLog );
        
        double tmpMin = costGraphs.at(snap.number() - 1).GetMinimum();
        double tmpMax = costGraphs.at(snap.number() - 1).GetMaximum();
        
        if (costLog.at(costLog.size() - 2) < minCost) {
            bestBasis = snap.number() - 1;
            minCost = costLog.at(costLog.size() - 2);
        }
        
        maxCost = (tmpMax > maxCost && tmpMax > 0 ? tmpMax : maxCost);
        //minCost = (tmpMin < minCost               ? tmpMin : minCost);
        
        
        // Filter graphs.
        double x[Ncoeffs], y[Ncoeffs];
        for (unsigned i = 0; i < Ncoeffs; i++) {
            x[i] = arma::as_scalar(filterLog.at(i).row(0));
            y[i] = arma::as_scalar(filterLog.at(i).row(1));
        }
        
        filterGraphs.at(snap.number() - 1) = TGraph(Ncoeffs, x, y);
        
        // Next.
        snap++;
        
    }
    
    cout << "Longest costlog = " << longestCost << endl;
    
    c.SetLogy(true);
    for (unsigned m = 0; m < M; m++) {
        if (costGraphs.at(m).GetN() == longestCost) {
            costGraphs.at(m).Draw("LAXIS");
            c.Update();
        }
    }
    
    for (unsigned m = 0; m < M; m++) {
        /*
        if (m == 0) {
            costGraphs.at(m).Draw("LAXIS");
            c.Update();
        }
         */
        //costGraphs.at(m).GetXaxis()->SetRangeUser(0, 200); //maxCost);
        costGraphs.at(m).GetYaxis()->SetRangeUser(0.001, 0.5); // Needle: (0.0, 0.5)
        //costGraphs.at(m).GetYaxis()->SetRangeUser(0.5, 0.7); // Uniform : (0.3, 0.55)
        costGraphs.at(m).SetLineColor(20 + m % 30);
        costGraphs.at(m).SetLineStyle(1);
        //costGraphs.at(m).Draw(m == 0 ? "LAXIS" : "L same");
        costGraphs.at(m).Draw("L same");
        c.Update();
    }
    c.RangeAxis(0,0.35,200,0.65);
    c.Update();
    c.SaveAs((outdir + "CostGraph" + ".pdf").c_str());
    
    
    // -- Colour palette.
    int kMyRed = 1756; // color index
    TColor *MyRed =  new TColor(kMyRed,  224./255.,   0./255.,  42./255.);
    int kMyBlue = 1757;
    TColor *MyBlue = new TColor(kMyBlue,   3./255.,  29./255.,  66./255.);
    
    const int Number = 3; // 2; // 
    double Red[Number]    = { 224./255., 0.98,   3./255.}; // {   3./255., 0.98 }; // 
    double Green[Number]  = {   0./255., 0.98,  29./255.}; // {  29./255., 0.98 }; // 
    double Blue[Number]   = {  42./255., 0.98,  66./255.}; // {  66./255., 0.98 }; // 
    double Length[Number] = { 0.00, 0.50, 1.00 }; // { 0.50, 1.00 }; // 
    int nb = 104; // 21;
    TColor::CreateGradientColorTable(Number, Length, Red, Green, Blue, nb);
    
    
    // * Get cost map(s).
    c.SetLogy(false);
    arma::Mat<double> costMap, costMapSparse, costMapReg;
    std::regex re (".N(\\d+)");
    std::string costMapName = "output/costMap." + std::regex_replace(project, re, "") + ".mat";
    std::string costMapRegName = "output/costMapReg." + std::regex_replace(project, re, "") + ".mat";
    std::string costMapSparseName = "output/costMapSparse." + std::regex_replace(project, re, "") + ".mat";

    
    
    if (!wavenet::fileExists(costMapName)) {
        arma::field< arma::Mat<double> > costs;
        
        costs = wavenet.costMap(examples, 1.2, 300);

        costMap = costs(0,0);
        costMap.save(costMapName);
        
        costMap = costs(1,0);
        costMap.save(costMapSparseName);

        costMap = costs(2,0);
        costMap.save(costMapRegName);
        
        costMap = costs(0,0);

    } else {
        costMap      .load(costMapName);
        costMapSparse.load(costMapSparseName);
        costMapReg   .load(costMapRegName);
    }
    
    
    c.SetLogz(true);
    std::unique_ptr<TH1> J = wavenet::MatrixToHist(costMap, 1.2);
    J->SetContour(104); // (104);
    gStyle->SetOptStat(0);
    J->SetMaximum(100.); // 100.
    //J->SetMinimum(0.001); // 0.033
    
    // * Styling.
    J->GetXaxis()->SetTitle("Filter coeff. a_{1}");
    J->GetYaxis()->SetTitle("Filter coeff. a_{2}");
    J->GetZaxis()->SetTitle("Cost (sparsity + regularisation) [a.u.]");
    
    J->GetXaxis()->SetTitleOffset(1.2);
    J->GetYaxis()->SetTitleOffset(1.3);
    J->GetZaxis()->SetTitleOffset(1.4);
    
    c.SetTopMargin   (0.09);
    c.SetBottomMargin(0.11);
    c.SetLeftMargin  (0.10 + (1/3.)*(1/7.));
    c.SetRightMargin (0.10 + (2/3.)*(1/7.));
    
    c.SetTickx();
    c.SetTicky();
    
    J->Draw("CONT1 Z"); // COL / CONT1
    c.Update();
    
    // * Lines etc.
    TEllipse normBoundary;
    normBoundary.SetFillStyle(0);
    normBoundary.SetLineStyle(2);
    normBoundary.SetLineColor(kMyRed);
    normBoundary.DrawEllipse(0., 0., 1., 1., 0., 360., 0.);

    TLine line;
    line.SetLineWidth(1);
    line.SetLineColor(kMyBlue);
    //line.DrawLine(-1.2, 0., 1.2, 0.);
    //line.DrawLine(0., -1.2, 0., 1.2);
    
    // * Markers.
    TMarker marker;
    for (unsigned m = 0; m < M; m++) {
        filterGraphs.at(m).Draw("L same");

        // * Marker
        double x, y;
        marker.SetMarkerColor(kRed);
        marker.SetMarkerStyle(8);
        marker.SetMarkerSize (0.3);
        filterGraphs.at(m).GetPoint(0, x, y);
        marker.DrawMarker(x, y);
        
        marker.SetMarkerColor(kBlue);
        marker.SetMarkerStyle(19);
        marker.SetMarkerSize (0.3);
        filterGraphs.at(m).GetPoint(filterGraphs.at(m).GetN() - 1, x, y);
        marker.DrawMarker(x, y);
    }
    
    
    // * Text
    TLatex text;
    text.SetTextFont(42);
    text.SetTextSize(0.035);
    switch (mode) {
        case wavenet::GeneratorMode::File:
            text.DrawLatexNDC(    c.GetLeftMargin(),  1. - c.GetTopMargin() + 0.025, "W (#rightarrow qq) + jets, #hat{p}_{T}  > 280 GeV");
            text.SetTextAlign(31);
            text.DrawLatexNDC(1 - c.GetRightMargin(), 1. - c.GetTopMargin() + 0.025, "#sqrt{s} = 13 TeV");
            break;
            
        default:
            break;
    }
    
    c.SaveAs((outdir + "CostMap.pdf").c_str());
    c.SetLogz(false);
    
    
     // * Basis function.
    // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
    snap.setNumber(bestBasis + 1);
    wavenet.load(snap.file());
    
    
    
    cout << "Checking orthonormality (best snap):" << endl;
    TH1F norms ("norms", "", 200, -0.5, 1.5);
    
    const unsigned int sizex = generator->shape()[0];
    const unsigned int sizey = generator->shape()[1];
    for (unsigned i = 0; i < wavenet::sq(sizex); i++) {
        for (unsigned j = 0; j < wavenet::sq(sizey); j++) {
            Mat<double> f1 = wavenet.basisFunction(sizex, sizey, i % sizex, j / sizey);
            Mat<double> f2 = wavenet.basisFunction(sizex, sizey, i % sizex, j / sizey);
            double norm = trace(f1*f2.t());
            norms.Fill( norm < -0.5 ? -0.499 : (norm > 1.5 ? 1.499 : norm) );
        }
    }
    
    c.SetLogy(true);
    norms.Draw("HIST");
    c.SaveAs((outdir + "NormDistributions.pdf").c_str());
    
    /* --- */
    unsigned dim  = 8;
    double   dimf = double(dim);
    double   marg = 0.03;

    unsigned dimx = std::min(sizex, dim);
    unsigned dimy = std::min(sizey, dim);

    double   dimfx = double(dimx);
    double   dimfy = double(dimy);

    TCanvas cBasis ("cBasis", "", 1200 * dimfx/dimf, 1200 * dimfy / dimf);
    
    //cBasis.Divide(dim, dim, 0.01, 0.01);
    
    
    vector< vector<TPad*> > pads (dim, vector<TPad*> (dim));

    for (unsigned i = 0; i < dimx; i++) {
        for (unsigned j = 0; j < dimy; j++) {
        pads[i][j] = new TPad((string("pad_") + to_string(i) + "_" + to_string(j)).c_str(), "", i/dimfx, (dimy - j - 1)/dimfy, (i+1)/dimfx, (dimy - j)/dimfy);
        pads[i][j]->SetMargin(marg, marg, marg, marg);
        pads[i][j]->SetTickx();
        pads[i][j]->SetTicky();
        cBasis.cd();
        pads[i][j]->Draw();
        }
    }

    
    /* --- */
    auto filterLog = wavenet.filterLog();
    unsigned Ncoeffs = filterLog.size();
    double zmax = 0.40;
    unsigned iFrame = 0;
    for (unsigned iCoeff = 0; iCoeff < Ncoeffs; iCoeff++) {
        auto filter = filterLog.at(iCoeff);
        wavenet.setFilter(filter);
        if (iCoeff > 100 and iCoeff < Ncoeffs - 100 and (iCoeff % 4 > 0)) { continue; } // Reduce number of frames.
        for (unsigned i = 0; i < dimx; i++) {
            for (unsigned j = 0; j < dimy; j++) {
                
                //cBasis.cd(1 + i * dim + j);
                pads[i][j]->cd();
                
                std::unique_ptr<TH1> basisFct = wavenet::MatrixToHist(wavenet.basisFunction(sizex, sizey, i, j), 3.2);
                
                basisFct->GetZaxis()->SetRangeUser(-zmax, zmax);
                basisFct->SetContour(nb);
                
                basisFct->GetXaxis()->SetTickLength(0.);
                basisFct->GetYaxis()->SetTickLength(0.);
                basisFct->GetXaxis()->SetTitleOffset(9999.);
                basisFct->GetYaxis()->SetTitleOffset(9999.);
                basisFct->GetXaxis()->SetLabelOffset(9999.);
                basisFct->GetYaxis()->SetLabelOffset(9999.);
                
                basisFct->DrawCopy("COL");
                
            }
        }
        
        char buff[100];
        snprintf(buff, sizeof(buff), (outdir + "movie/bestBasis_%06d.png").c_str(), iFrame++); // iCoeff);
        std::string buffAsStdStr = buff;
        
        cBasis.SaveAs(buffAsStdStr.c_str());
        
    }

     for (unsigned i = 0; i < dim; i++) {
        for (unsigned j = 0; j < dim; j++) {
            delete pads[i][j]; pads[i][j] = nullptr;
        }
    }

    /* -- */

    
    /* --- * /
     basisFct = MatrixToHist(wavenet.basisFct(64, 0, 0), 3.2);
     basisFct.Draw("COL Z");
     c.SaveAs((outdir + "BestBasisFunction-0-0.pdf").c_str());
     
     basisFct = MatrixToHist(wavenet.basisFct(64, 1, 1), 3.2);
     basisFct.Draw("COL Z");
     c.SaveAs((outdir + "BestBasisFunction-1-1.pdf").c_str());
     
     basisFct = MatrixToHist(wavenet.basisFct(64, 2, 2), 3.2);
     basisFct.Draw("COL Z");
     c.SaveAs((outdir + "BestBasisFunction-2-2.pdf").c_str());
     
     basisFct = MatrixToHist(wavenet.basisFct(64, 6, 6), 3.2);
     basisFct.Draw("COL Z");
     c.SaveAs((outdir + "BestBasisFunction-6-6.pdf").c_str());
     
     basisFct = MatrixToHist(wavenet.basisFct(64, 6, 40), 3.2);
     basisFct.Draw("COL Z");
     c.SaveAs((outdir + "BestBasisFunction-6-40.pdf").c_str());
     
     basisFct = MatrixToHist(wavenet.basisFct(64, 40, 40), 3.2);
     basisFct.Draw("COL Z");
     c.SaveAs((outdir + "BestBasisFunction-40-40.pdf").c_str());
     / * --- */
    cout << "Done." << endl;
    
    return 1;
}

