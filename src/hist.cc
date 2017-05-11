#include <iostream>
// #include <iomanip>
// #include <vector>
// #include <experimental/array>
// #include <memory>

#include <TFile.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TH1.h>
#include <TKey.h>

#include "timed_counter.hh"

#define TEST(VAR) \
    std::cout <<"\033[36m"<< #VAR <<"\033[0m"<< " = " << VAR << std::endl;

using std::cout;
using std::cerr;
using std::endl;

inline TH1D* make_hist(const char* name, std::initializer_list<double> bins) {
  return new TH1D(name,"",bins.size(),&*bins.begin());
}

int main(int argc, char* argv[]) {
  if (argc<3) {
    cout << "usage: " << argv[0] << " out.root MxAOD.root ..." << endl;
    return 1;
  }

  TFile fout(argv[1],"recreate");
  if (fout.IsZombie()) {
    cerr << "cannot open output file " << argv[1] << endl;
    return 1;
  }

  // Define histograms ----------------------------------------------
#define h_(name,n) TH1D *h_##name = new TH1D(#name,"",n,b_##name);

  const double b_N_j[] = { 0,1,2,3 };
  const double b_pT_yy[] = { 0,20,30,45,60,80,120,170,220,350 };
  const double b_pT_j1[] = { 30,55,75,120,350 };
  const double b_m_jj[] = { 0.,170.,500.,1500. };
  const double b_Dphi_j_j[] = { 0.,1.0472,2.0944,3.15 };
  const double b_Dphi_j_j_signed[] = { -3.15,-1.570796,0,1.570796,3.15 };

  h_(N_j,3)
  h_(pT_yy,9)
  h_(pT_j1,4)
  h_(m_jj,3)
  h_(Dphi_j_j,3)
  h_(Dphi_j_j_signed,4)
  // ----------------------------------------------------------------
  
  for (int a=2; a<argc; ++a) { // loop over MxAODs
    TFile file(argv[a]);
    if (file.IsZombie()) {
      cerr << "cannot open input file " << argv[a] << endl;
      return 1;
    }
    double factor = 1;

    TIter next(file.GetListOfKeys());
    for (TKey *key; (key = static_cast<TKey*>(next())); ) {
      std::string name(key->GetName());
      if (name.substr(0,8)!="CutFlow_" ||
          name.substr(name.size()-18)!="_noDalitz_weighted") continue;
      TH1 *h = static_cast<TH1*>(key->ReadObj());
      cout << h->GetName() << endl;
      const double n_all = h->GetBinContent(3);
      cout << h->GetXaxis()->GetBinLabel(3) << " = " << n_all << endl;
      factor = 1./n_all;
      break;
    }

    // read variables ===============================================
    TTreeReader reader("CollectionTree",&file);
    TTreeReaderValue<Float_t> _cs_br_fe(reader,"HGamEventInfoAuxDyn.crossSectionBRfilterEff");
    TTreeReaderValue<Float_t> _weight(reader,"HGamEventInfoAuxDyn.weight");
    TTreeReaderValue<Char_t> _isFiducial(reader,"HGamTruthEventInfoAuxDyn.isFiducial");
    // TTreeReaderValue<Char_t> _isPassed(reader,"HGamEventInfoAuxDyn.isPassed");

#define VAR_GEN_(NAME, TYPE, STR) \
  TTreeReaderValue<TYPE> _##NAME(reader, "HGamTruthEventInfoAuxDyn." STR);
#define VAR_(NAME) VAR_GEN_(NAME, Float_t, #NAME)
#define VAR30_(NAME) VAR_GEN_(NAME, Float_t, #NAME "_30")

    TTreeReaderValue<std::vector<std::vector<float>>> _ww(reader,"TruthEventsAuxDyn.weights");

    VAR_GEN_(N_j, Int_t, "N_j_30")

    VAR_(m_yy)
    VAR_(pT_yy)
    // VAR_(yAbs_yy)
    // VAR_(cosTS_yy)
    // VAR_(pTt_yy)
    // VAR_(Dy_y_y)

    // VAR30_(HT)
    VAR30_(pT_j1) //      VAR30_(pT_j2)      VAR30_(pT_j3)
    // VAR30_(yAbs_j1)    VAR30_(yAbs_j2)
    VAR30_(Dphi_j_j)   VAR_GEN_(Dphi_j_j_signed,Float_t,"Dphi_j_j_30_signed")
    // VAR30_(Dy_j_j)
    VAR30_(m_jj)
    // VAR30_(sumTau_yyj) VAR30_(maxTau_yyj)
    // VAR30_(pT_yyjj)    VAR30_(Dphi_yy_jj)

    // loop over events =============================================
    using tc = ivanp::timed_counter<Long64_t>;
    for (tc ent(reader.GetEntries(true)); reader.Next(); ++ent) {

      if (ent<5) {
        if (ent==0) cout << endl;
        auto ww = *_ww;
        TEST( ww.size() )
        TEST( ww[0].size() )
      }

      // selection cut
      // if (!*_isPassed) continue;
      if (!*_isFiducial) continue;

      // diphoton mass cut
      const auto m_yy = *_m_yy;
      if (m_yy < 105e3 || 160e3 < m_yy) continue;

      const double weight = (*_weight) * (*_cs_br_fe) * factor;

      // FILL HISTOGRAMS ============================================
      h_N_j->Fill(*_N_j,weight);
      h_pT_yy->Fill(*_pT_yy*1e-3,weight);
      h_pT_j1->Fill(*_pT_j1*1e-3,weight);
      h_m_jj->Fill(*_m_jj*1e-3,weight);
      h_Dphi_j_j->Fill(*_Dphi_j_j,weight);
      h_Dphi_j_j_signed->Fill(*_Dphi_j_j_signed,weight);
    }

    file.Close();
  }

  fout.Write();

  return 0;
}
