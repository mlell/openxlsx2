#include "openxlsx2.h"
#include <fstream>

// [[Rcpp::export]]
Rcpp::CharacterVector set_sst(Rcpp::CharacterVector sharedStrings) {

  Rcpp::CharacterVector sst(sharedStrings.length());

  for (auto i = 0; i < sharedStrings.length(); ++i) {
    pugi::xml_document si;
    std::string sharedString = Rcpp::as<std::string>(sharedStrings[i]);

    si.append_child("si").append_child("t").append_child(pugi::node_pcdata).set_value(sharedString.c_str());

    std::ostringstream oss;
    si.print(oss, " ", pugi::format_raw);

    sst[i] = oss.str();
  }

  return sst;
}


// helper function to access element from Rcpp::Character Vector as string
std::string to_string(Rcpp::Vector<16>::Proxy x) {
  return Rcpp::as<std::string>(x);
}


// creates an xml row
// data in xml is ordered row wise. therefore we need the row attributes and
// the column data used in this row. This function uses both to create a single
// row and passes it to write_worksheet_xml_2 which will create the entire
// sheet_data part for this worksheet
std::string xml_sheet_data(Rcpp::DataFrame row_attr, Rcpp::DataFrame cc) {

  auto lastrow = 0; // integer value of the last row with column data
  auto thisrow = 0; // integer value of the current row with column data
  auto row_idx = 0; // the index of the row_attr file. this is != rowid
  auto rowid   = 0; // integer value of the r field in row_attr

  pugi::xml_document doc;
  pugi::xml_node row;

  std::string rnastring = "_openxlsx_NA_";
  std::string xml_preserver = "";

  // non optional: treat input as valid at this stage
  unsigned int pugi_parse_flags = pugi::parse_cdata | pugi::parse_wconv_attribute | pugi::parse_eol;
  // unsigned int pugi_format_flags = pugi::format_raw | pugi::format_no_escapes;

  // we cannot access rows directly in the dataframe.
  // Have to extract the columns and use these
  Rcpp::CharacterVector cc_row_r = cc["row_r"]; // 1
  Rcpp::CharacterVector cc_r     = cc["r"];     // A1
  Rcpp::CharacterVector cc_v     = cc["v"];
  Rcpp::CharacterVector cc_c_t   = cc["c_t"];
  Rcpp::CharacterVector cc_c_s   = cc["c_s"];
  Rcpp::CharacterVector cc_f     = cc["f"];
  Rcpp::CharacterVector cc_f_t   = cc["f_t"];
  Rcpp::CharacterVector cc_f_ref = cc["f_ref"];
  Rcpp::CharacterVector cc_f_ca  = cc["f_ca"];
  Rcpp::CharacterVector cc_f_si  = cc["f_si"];
  Rcpp::CharacterVector cc_is    = cc["is"];

  Rcpp::CharacterVector row_r    = row_attr["r"];

  for (auto i = 0; i < cc.nrow(); ++i) {

    thisrow = std::stoi(Rcpp::as<std::string>(cc_row_r[i]));

    if (lastrow < thisrow) {

      // there might be entirely empty rows in between. this is the case for
      // loadExample. We check the rowid and write the line and skip until we
      // have every row and only then continue writing the column
      while (rowid < thisrow) {

        rowid = std::stoi(Rcpp::as<std::string>(
          row_r[row_idx]
        ));

        row = doc.append_child("row");
        Rcpp::CharacterVector attrnams = row_attr.names();

        for (auto j = 0; j < row_attr.ncol(); ++j) {

          Rcpp::CharacterVector cv_s = "";
          cv_s = Rcpp::as<Rcpp::CharacterVector>(row_attr[j])[row_idx];

          if (cv_s[0] != "") {
            const std::string val_strl = Rcpp::as<std::string>(cv_s);
            row.append_attribute(attrnams[j]) = val_strl.c_str();
          }
        }

        // read the next row_idx when visiting again
        ++row_idx;
      }
    }

    // create node <c>
    pugi::xml_node cell = row.append_child("c");

    // Every cell consists of a typ and a val list. Certain functions have an
    // additional attr list.

    // append attributes <c r="A1" ...>
    cell.append_attribute("r") = to_string(cc_r[i]).c_str();

    if (to_string(cc_c_s[i]).compare(rnastring.c_str()) != 0)
      cell.append_attribute("s") = to_string(cc_c_s[i]).c_str();

    // assign type if not <v> aka numeric
    if (to_string(cc_c_t[i]).compare(rnastring.c_str()) != 0)
      cell.append_attribute("t") = to_string(cc_c_t[i]).c_str();

    // append nodes <c r="A1" ...><v>...</v></c>

    bool f_si = false;

    // <f> ... </f>
    // f node: formula to be evaluated
    if (to_string(cc_f[i]).compare(rnastring.c_str()) != 0 || to_string(cc_f_t[i]).compare(rnastring.c_str()) != 0 || to_string(cc_f_si[i]).compare(rnastring.c_str()) != 0) {
      pugi::xml_node f = cell.append_child("f");
      if (to_string(cc_f_t[i]).compare(rnastring.c_str()) != 0) {
        f.append_attribute("t") = to_string(cc_f_t[i]).c_str();
      }
      if (to_string(cc_f_ref[i]).compare(rnastring.c_str()) != 0) {
        f.append_attribute("ref") = to_string(cc_f_ref[i]).c_str();
      }
      if (to_string(cc_f_ca[i]).compare(rnastring.c_str()) != 0) {
        f.append_attribute("ca") = to_string(cc_f_ca[i]).c_str();
      }
      if (to_string(cc_f_si[i]).compare(rnastring.c_str()) != 0) {
        f.append_attribute("si") = to_string(cc_f_si[i]).c_str();
        f_si = true;
      }

      f.append_child(pugi::node_pcdata).set_value(to_string(cc_f[i]).c_str());
    }

    // v node: value stored from evaluated formula
    if (to_string(cc_v[i]).compare(rnastring.c_str()) != 0) {
      if (!f_si & (to_string(cc_v[i]).compare(xml_preserver.c_str()) == 0)) {
        cell.append_child("v").append_attribute("xml:space").set_value("preserve");
        cell.child("v").append_child(pugi::node_pcdata).set_value(" ");
      } else {
        cell.append_child("v").append_child(pugi::node_pcdata).set_value(to_string(cc_v[i]).c_str());
      }
    }

    // <is><t> ... </t></is>
    if(to_string(cc_c_t[i]).compare("inlineStr") == 0) {
      if (to_string(cc_is[i]).compare(rnastring.c_str()) != 0) {

        pugi::xml_document is_node;
        pugi::xml_parse_result result = is_node.load_string(to_string(cc_is[i]).c_str(), pugi_parse_flags);
        if (!result) Rcpp::stop("loading inlineStr node while writing failed");

        cell.append_copy(is_node.first_child());
      }
    }

    // update lastrow
    lastrow = thisrow;
  }

  std::ostringstream oss;
  doc.print(oss, " ", pugi::format_raw | pugi::format_no_escapes);
  return oss.str();
}


// TODO: convert to pugi
// function that creates the xml worksheet
// uses preparated data and writes it. It passes data to set_row() which will
// create single xml rows of sheet_data.
//
// [[Rcpp::export]]
void write_worksheet(std::string prior,
                     std::string post,
                     Rcpp::Environment sheet_data,
                     Rcpp::CharacterVector cols_attr, // currently unused
                     std::string R_fileName = "output") {


  // open file and write header XML
  const char * s = R_fileName.c_str();
  std::ofstream xmlFile;
  xmlFile.open (s);
  xmlFile << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n";
  xmlFile << prior;

  // sheet_data will be in order, just need to check for row_heights
  // CharacterVector cell_col = int_to_col(sheet_data.field("cols"));
  Rcpp::DataFrame row_attr = Rcpp::as<Rcpp::DataFrame>(sheet_data["row_attr"]);
  Rcpp::DataFrame cc = Rcpp::as<Rcpp::DataFrame>(sheet_data["cc_out"]);

  // TODO prev. this was Rf_isNull() no we have a zero col, zero row dataframe?
  if ((row_attr.nrow() == 0) || (cc.nrow() == 0)) {
    xmlFile << "<sheetData />";
  } else {

    xmlFile << "<sheetData>";

    // cc to sheet_data
    xmlFile << xml_sheet_data(row_attr, cc);

    // write closing tag and XML post data
    xmlFile << "</sheetData>";

  }

  xmlFile << post;

  //close file
  xmlFile.close();

}
