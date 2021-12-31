#include "openxlsx2.h"

// [[Rcpp::export]]
Rcpp::DataFrame read_xf(XPtrXML xml_doc_xf) {

  // https://docs.microsoft.com/en-us/dotnet/api/documentformat.openxml.spreadsheet.cellformat?view=openxml-2.8.1

  // openxml 2.8.1
  Rcpp::CharacterVector nams = {
    "applyAlignment",
    "applyBorder",
    "applyFill",
    "applyFont",
    "applyNumberFormat",
    "applyProtection",
    "borderId",
    "fillId",
    "fontId",
    "numFmtId",
    "pivotButton",
    "quotePrefix",
    "xfId",
    // child alignment
    "horizontal",
    "indent",
    "justifyLastLine",
    "readingOrder",
    "relativeIndent",
    "shrinkToFit",
    "textRotation",
    "vertical",
    "wrapText",
    // child extLst
    "extLst",
    // child protection
    "hidden",
    "locked"
  };


  auto nn = std::distance(xml_doc_xf->begin(), xml_doc_xf->end());
  auto kk = nams.length();

  Rcpp::IntegerVector rvec = Rcpp::seq_len(nn);

  // 1. create the list
  Rcpp::List df(kk);
  for (auto i = 0; i < kk; ++i)
  {
    SET_VECTOR_ELT(df, i, Rcpp::CharacterVector(Rcpp::no_init(nn)));
  }

  // 2. fill the list
  // <xf ...>
  auto itr = 0;
  for (auto xml_xf : xml_doc_xf->children()) {

    std::string xf_name = xml_xf.name();
    if (xf_name != "xf")
      Rcpp::stop("xml_node is not xf");

    for (auto attrs : xml_xf.attributes()) {

      Rcpp::CharacterVector attr_name = attrs.name();
      std::string attr_value = attrs.value();

      // mimic which
      Rcpp::IntegerVector mtc = Rcpp::match(nams, attr_name);
      Rcpp::IntegerVector idx = Rcpp::seq(0, mtc.length()-1);

      // check if name is already known
      if (all(Rcpp::is_na(mtc))) {
        Rcpp::Rcout << attr_name << ": not found in xf name table" << std::endl;
      } else {
        size_t ii = Rcpp::as<size_t>(idx[!Rcpp::is_na(mtc)]);
        Rcpp::as<Rcpp::CharacterVector>(df[ii])[itr] = attr_value;
      }

    }

    // only handle known names
    // <alignment ...>
    // <extLst ...>
    // <protection ...>
    for (auto cld : xml_xf.children()) {

      std::string cld_name = cld.name();

      // check known names
      if (cld_name ==  "alignment" | cld_name == "extLst" | cld_name == "protection") {

        for (auto attrs : cld.attributes()) {

          Rcpp::CharacterVector attr_name = attrs.name();
          std::string attr_value = attrs.value();

          // mimic which
          Rcpp::IntegerVector mtc = Rcpp::match(nams, attr_name);
          Rcpp::IntegerVector idx = Rcpp::seq(0, mtc.length()-1);

          // check if name is already known
          if (all(Rcpp::is_na(mtc))) {
            Rcpp::Rcout << attr_name << ": not found in xf name table" << std::endl;
          } else {
            size_t ii = Rcpp::as<size_t>(idx[!Rcpp::is_na(mtc)]);
            Rcpp::as<Rcpp::CharacterVector>(df[ii])[itr] = attr_value;
          }

        }

      } else {
        Rcpp::Rcout << cld_name << ": not valid for xf in openxml 2.8.1" << std::endl;
      }

    } // end aligment, extLst, protection

    ++itr;
  }


  // 3. Create a data.frame
  df.attr("row.names") = rvec;
  df.attr("names") = nams;
  df.attr("class") = "data.frame";

  return df;
}

// helper function to check if row contains any of the expected types
bool has_it(Rcpp::DataFrame df_xf, Rcpp::CharacterVector xf_nams, size_t row) {

  bool has_it = false;
  Rcpp::CharacterVector attrnams = df_xf.names();

  Rcpp::DataFrame df_tmp;
  Rcpp::CharacterVector cv_tmp;

  Rcpp::IntegerVector mtc = Rcpp::match(attrnams, xf_nams);
  Rcpp::IntegerVector idx = Rcpp::seq(0, mtc.length()-1);

  idx = idx[!Rcpp::is_na(mtc)];
  df_tmp = df_xf[idx];

  for (auto ii = 0; ii < df_tmp.ncol(); ++ii) {
    cv_tmp = Rcpp::as<Rcpp::CharacterVector>(df_tmp[ii])[row];
    if (cv_tmp[0] != "") has_it = true;
  }

  return has_it;
}


// [[Rcpp::export]]
Rcpp::CharacterVector write_xf(Rcpp::DataFrame df_xf) {

  auto n = df_xf.nrow();
  Rcpp::CharacterVector z(n);

  Rcpp::CharacterVector xf_nams = {
    "applyAlignment",
    "applyBorder",
    "applyFill",
    "applyFont",
    "applyNumberFormat",
    "applyProtection",
    "borderId",
    "fillId",
    "fontId",
    "numFmtId",
    "pivotButton",
    "quotePrefix",
    "xfId"
  };

  Rcpp::CharacterVector xf_nams_alignment = {
    "horizontal",
    "indent",
    "justifyLastLine",
    "readingOrder",
    "relativeIndent",
    "shrinkToFit",
    "textRotation",
    "vertical",
    "wrapText"
  };

  Rcpp::CharacterVector xf_nams_extLst = {
    "extLst"
  };

  Rcpp::CharacterVector xf_nams_protection = {
    "hidden",
    "locked"
  };

  for (auto i = 0; i < n; ++i) {
    pugi::xml_document doc;

    pugi::xml_node xf = doc.append_child("xf");
    Rcpp::CharacterVector attrnams = df_xf.names();

    // check if alignment node is required
    bool has_alignment = false;
    has_alignment = has_it(df_xf, xf_nams_alignment, i);

    pugi::xml_node xf_alignment;
    if (has_alignment) {
      xf_alignment = xf.append_child("alignment");
    }

    // check if extLst node is required
    bool has_extLst = false;
    has_extLst = has_it(df_xf, xf_nams_extLst, i);

    pugi::xml_node xf_extLst;
    if (has_extLst) {
      xf_extLst = xf.append_child("extLst");
    }

    // check if protection node is required
    bool has_protection = false;
    has_protection = has_it(df_xf, xf_nams_protection, i);

    pugi::xml_node xf_protection;
    if (has_protection) {
      xf_protection = xf.append_child("protection");
    }

    for (auto j = 0; j < df_xf.ncol(); ++j) {

      Rcpp::CharacterVector attrnam = Rcpp::as<Rcpp::CharacterVector>(attrnams[j]);

      // not all missing in match: ergo they are
      bool is_xf = !Rcpp::as<bool>(Rcpp::all(Rcpp::is_na(Rcpp::match(xf_nams, attrnam))));
      bool is_alignment = !Rcpp::as<bool>(Rcpp::all(Rcpp::is_na(Rcpp::match(xf_nams_alignment, attrnam))));
      bool is_extLst = !Rcpp::as<bool>(Rcpp::all(Rcpp::is_na(Rcpp::match(xf_nams_extLst, attrnam))));
      bool is_protection = !Rcpp::as<bool>(Rcpp::all(Rcpp::is_na(Rcpp::match(xf_nams_protection, attrnam))));

      // <xf ...>
      if (is_xf) {

        Rcpp::CharacterVector cv_s = "";
        cv_s = Rcpp::as<Rcpp::CharacterVector>(df_xf[j])[i];

        // only write attributes where cv_s has a value
        if (cv_s[0] != "") {
          // Rf_PrintValue(cv_s);
          const std::string val_strl = Rcpp::as<std::string>(cv_s);
          xf.append_attribute(attrnams[j]) = val_strl.c_str();
        }
      }

      if (has_alignment && is_alignment) {

        Rcpp::CharacterVector cv_s = "";
        cv_s = Rcpp::as<Rcpp::CharacterVector>(df_xf[j])[i];

        if (cv_s[0] != "") {
          const std::string val_strl = Rcpp::as<std::string>(cv_s);
          xf_alignment.append_attribute(attrnams[j]) = val_strl.c_str();
        }

      }

      if (has_extLst && is_extLst) {

        Rcpp::CharacterVector cv_s = "";
        cv_s = Rcpp::as<Rcpp::CharacterVector>(df_xf[j])[i];

        if (cv_s[0] != "") {
          const std::string val_strl = Rcpp::as<std::string>(cv_s);
          xf_extLst.append_attribute(attrnams[j]) = val_strl.c_str();
        }

      }

      if (has_protection && is_protection) {

        Rcpp::CharacterVector cv_s = "";
        cv_s = Rcpp::as<Rcpp::CharacterVector>(df_xf[j])[i];

        if (cv_s[0] != "") {
          const std::string val_strl = Rcpp::as<std::string>(cv_s);
          xf_protection.append_attribute(attrnams[j]) = val_strl.c_str();
        }

      }

    }

    std::ostringstream oss;
    doc.print(oss, " ", pugi::format_raw);

    z[i] = oss.str();
  }

  return z;
}

