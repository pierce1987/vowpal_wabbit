// #include "pylibvw.h"
#include "../vowpalwabbit/vw.h"
#include "../vowpalwabbit/multiclass.h"
#include "../vowpalwabbit/cost_sensitive.h"
#include "../vowpalwabbit/cb.h"
#include "../vowpalwabbit/searn.h"
#include "../vowpalwabbit/searn_pythontask.h"

#include <boost/make_shared.hpp>
#include <boost/python.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

using namespace std;
namespace py=boost::python;

typedef boost::shared_ptr<vw> vw_ptr;
typedef boost::shared_ptr<example> example_ptr;
typedef boost::shared_ptr<Searn::searn> searn_ptr;

void dont_delete_me(void*arg) {}

vw_ptr my_initialize(string args) {
  vw*foo = VW::initialize(args);
  return boost::shared_ptr<vw>(foo, dont_delete_me);
}

void my_finish(vw_ptr all) {
  VW::finish(*all, false);  // don't delete all because python will do that for us!
}

searn_ptr get_searn_ptr(vw_ptr all) {
  return boost::shared_ptr<Searn::searn>((Searn::searn*)(all->searnstr), dont_delete_me);
}


example_ptr my_read_example(vw_ptr all, char*str) {
  example*ec = VW::read_example(*all, str);
  return boost::shared_ptr<example>(ec, dont_delete_me);
}

void my_finish_example(vw_ptr all, example_ptr ec) {
  VW::finish_example(*all, ec.get());
}

void my_learn(vw_ptr all, example_ptr ec) {
  all->learn(ec.get());
}

float my_learn_string(vw_ptr all, char*str) {
  example*ec = VW::read_example(*all, str);
  all->learn(ec);
  float pp = ec->partial_prediction;
  VW::finish_example(*all, ec);
  return pp;
}

string varray_char_to_string(v_array<char> &a) {
  string ret = "";
  for (char*c = a.begin; c != a.end; ++c)
    ret += *c;
  return ret;
}

string my_get_tag(example_ptr ec) {
  return varray_char_to_string(ec->tag);
}

uint32_t ex_num_namespaces(example_ptr ec) {
  return ec->indices.size();
}

unsigned char ex_namespace(example_ptr ec, uint32_t ns) {
  return ec->indices[ns];
}

uint32_t ex_num_features(example_ptr ec, unsigned char ns) {
  return ec->atomics[ns].size();
}

uint32_t ex_feature(example_ptr ec, unsigned char ns, uint32_t i) {
  return ec->atomics[ns][i].weight_index;
}

float ex_feature_weight(example_ptr ec, unsigned char ns, uint32_t i) {
  return ec->atomics[ns][i].x;
}

float ex_sum_feat_sq(example_ptr ec, unsigned char ns) {
  return ec->sum_feat_sq[ns];
}

void ex_push_feature(example_ptr ec, unsigned char ns, uint32_t fid, float v) {
  // warning: assumes namespace exists!
  feature f = { v, fid };
  ec->atomics[ns].push_back(f);
  ec->num_features++;
  ec->sum_feat_sq[ns] += v * v;
  ec->total_sum_feat_sq += v * v;
}

bool ex_pop_feature(example_ptr ec, unsigned char ns) {
  if (ec->atomics[ns].size() == 0) return false;
  feature f = ec->atomics[ns].pop();
  ec->num_features--;
  ec->sum_feat_sq[ns] -= f.x * f.x;
  ec->total_sum_feat_sq -= f.x * f.x;
  return true;
}

void ex_push_namespace(example_ptr ec, unsigned char ns) {
  ec->indices.push_back(ns);
}

void ex_ensure_namespace_exists(example_ptr ec, unsigned char ns) {
  for (unsigned char* nss = ec->indices.begin; nss != ec->indices.end; ++nss)
    if (ns == *nss) return;
  ex_push_namespace(ec, ns);
}

bool ex_pop_namespace(example_ptr ec) {
  if (ec->indices.size() == 0) return false;
  unsigned char ns = ec->indices.pop();
  ec->num_features -= ec->atomics[ns].size();
  ec->total_sum_feat_sq -= ec->sum_feat_sq[ns];
  ec->sum_feat_sq[ns] = 0.;
  ec->atomics[ns].erase();
  return true;
}

void my_setup_example(vw_ptr vw, example_ptr ec) {
  VW::setup_example(*vw, ec.get());
}

example_ptr my_empty_example(vw_ptr vw) {
  example* new_ec = VW::new_unused_example(*vw);
  vw->p->lp.default_label(new_ec->ld);
  return boost::shared_ptr<example>(new_ec, dont_delete_me);
}  

void ex_set_label_string(example_ptr ec, vw_ptr vw, string label) {
  VW::parse_example_label(*vw, *ec, label);
}

float ex_get_simplelabel_label(example_ptr ec) { return ((label_data*)ec->ld)->label; }
float ex_get_simplelabel_weight(example_ptr ec) { return ((label_data*)ec->ld)->weight; }
float ex_get_simplelabel_initial(example_ptr ec) { return ((label_data*)ec->ld)->initial; }
float ex_get_simplelabel_prediction(example_ptr ec) { return ((label_data*)ec->ld)->prediction; }

uint32_t ex_get_multiclass_label(example_ptr ec) { return ((MULTICLASS::multiclass*)ec->ld)->label; }
float ex_get_multiclass_weight(example_ptr ec) { return ((MULTICLASS::multiclass*)ec->ld)->weight; }
uint32_t ex_get_multiclass_prediction(example_ptr ec) { return ((MULTICLASS::multiclass*)ec->ld)->prediction; }

uint32_t ex_get_costsensitive_prediction(example_ptr ec) { return ((COST_SENSITIVE::label*)ec->ld)->prediction; }
uint32_t ex_get_costsensitive_num_costs(example_ptr ec) { return ((COST_SENSITIVE::label*)ec->ld)->costs.size(); }
float ex_get_costsensitive_cost(example_ptr ec, uint32_t i) { return ((COST_SENSITIVE::label*)ec->ld)->costs[i].x; }
uint32_t ex_get_costsensitive_class(example_ptr ec, uint32_t i) { return ((COST_SENSITIVE::label*)ec->ld)->costs[i].class_index; }
float ex_get_costsensitive_partial_prediction(example_ptr ec, uint32_t i) { return ((COST_SENSITIVE::label*)ec->ld)->costs[i].partial_prediction; }
float ex_get_costsensitive_wap_value(example_ptr ec, uint32_t i) { return ((COST_SENSITIVE::label*)ec->ld)->costs[i].wap_value; }

uint32_t ex_get_cbandits_prediction(example_ptr ec) { return ((CB::label*)ec->ld)->prediction; }
uint32_t ex_get_cbandits_num_costs(example_ptr ec) { return ((CB::label*)ec->ld)->costs.size(); }
float ex_get_cbandits_cost(example_ptr ec, uint32_t i) { return ((CB::label*)ec->ld)->costs[i].cost; }
uint32_t ex_get_cbandits_class(example_ptr ec, uint32_t i) { return ((CB::label*)ec->ld)->costs[i].action; }
float ex_get_cbandits_probability(example_ptr ec, uint32_t i) { return ((CB::label*)ec->ld)->costs[i].probability; }
float ex_get_cbandits_partial_prediction(example_ptr ec, uint32_t i) { return ((CB::label*)ec->ld)->costs[i].partial_prediction; }

size_t   get_example_counter(example_ptr ec) { return ec->example_counter; }
uint32_t get_ft_offset(example_ptr ec) { return ec->ft_offset; }
size_t   get_num_features(example_ptr ec) { return ec->num_features; }
float    get_partial_prediction(example_ptr ec) { return ec->partial_prediction; }
float    get_updated_prediction(example_ptr ec) { return ec->updated_prediction; }
float    get_loss(example_ptr ec) { return ec->loss; }
float    get_example_t(example_ptr ec) { return ec->example_t; }
float    get_total_sum_feat_sq(example_ptr ec) { return ec->total_sum_feat_sq; }

double get_sum_loss(vw_ptr vw) { return vw->sd->sum_loss; }
double get_weighted_examples(vw_ptr vw) { return vw->sd->weighted_examples; }

bool searn_should_output(searn_ptr srn) { return srn->output().good(); }
void searn_output(searn_ptr srn, string s) { srn->output() << s; }

uint32_t searn_predict_one_all(searn_ptr srn, example_ptr ec, uint32_t one_ystar) {
  return srn->predict(ec.get(), one_ystar, NULL);
}

uint32_t searn_predict_one_some(searn_ptr srn, example_ptr ec, uint32_t one_ystar, vector<uint32_t>& yallowed) {
  v_array<uint32_t> yallowed_va;
  yallowed_va.begin       = yallowed.data();
  yallowed_va.end         = yallowed_va.begin + yallowed.size();
  yallowed_va.end_array   = yallowed_va.end;
  yallowed_va.erase_count = 0;
  return srn->predict(ec.get(), one_ystar, &yallowed_va);
}

uint32_t searn_predict_many_all(searn_ptr srn, example_ptr ec, vector<uint32_t>& ystar) {
  v_array<uint32_t> ystar_va;
  ystar_va.begin       = ystar.data();
  ystar_va.end         = ystar_va.begin + ystar.size();
  ystar_va.end_array   = ystar_va.end;
  ystar_va.erase_count = 0;
  return srn->predict(ec.get(), &ystar_va, NULL);
}

uint32_t searn_predict_many_some(searn_ptr srn, example_ptr ec, vector<uint32_t>& ystar, vector<uint32_t>& yallowed) {
  v_array<uint32_t> ystar_va;
  ystar_va.begin       = ystar.data();
  ystar_va.end         = ystar_va.begin + ystar.size();
  ystar_va.end_array   = ystar_va.end;
  ystar_va.erase_count = 0;
  v_array<uint32_t> yallowed_va;
  yallowed_va.begin       = yallowed.data();
  yallowed_va.end         = yallowed_va.begin + yallowed.size();
  yallowed_va.end_array   = yallowed_va.end;
  yallowed_va.erase_count = 0;
  return srn->predict(ec.get(), &ystar_va, &yallowed_va);
}

/* IN ORDER TO DO THIS, WE NEED TO MAKE LDF BACK TO example** :(
uint32_t searn_predictLDF_one_all(searn_ptr srn, vector<example*> ecs, uint32_t one_ystar) {
  return srn->predictLDF(ecs.data(), ecs.size(), one_ystar, NULL);
}

uint32_t searn_predictLDF_one_some(searn_ptr srn, example_ptr ec, size_t ec_len, uint32_t one_ystar, vector<uint32_t>& yallowed) {
  v_array<uint32_t> yallowed_va;
  yallowed_va.begin       = yallowed.data();
  yallowed_va.end         = yallowed_va.begin + yallowed.size();
  yallowed_va.end_array   = yallowed_va.end;
  yallowed_va.erase_count = 0;
  return srn->predictLDF(ec.get(), one_ystar, &yallowed_va);
}

uint32_t searn_predictLDF_many_all(searn_ptr srn, example_ptr ec, size_t ec_len, vector<uint32_t>& ystar) {
  v_array<uint32_t> ystar_va;
  ystar_va.begin       = ystar.data();
  ystar_va.end         = ystar_va.begin + ystar.size();
  ystar_va.end_array   = ystar_va.end;
  ystar_va.erase_count = 0;
  return srn->predictLDF(ec.get(), &ystar_va, NULL);
}

uint32_t searn_predictLDF_many_some(searn_ptr srn, example_ptr ec, size_t ec_len, vector<uint32_t>& ystar, vector<uint32_t>& yallowed) {
  v_array<uint32_t> ystar_va;
  ystar_va.begin       = ystar.data();
  ystar_va.end         = ystar_va.begin + ystar.size();
  ystar_va.end_array   = ystar_va.end;
  ystar_va.erase_count = 0;
  v_array<uint32_t> yallowed_va;
  yallowed_va.begin       = yallowed.data();
  yallowed_va.end         = yallowed_va.begin + yallowed.size();
  yallowed_va.end_array   = yallowed_va.end;
  yallowed_va.erase_count = 0;
  return srn->predictLDF(ec.get(), &ystar_va, &yallowed_va);
}
*/





void verify_searn_set_properly(searn_ptr srn) {
  if ((srn->task == NULL) || (srn->task->task_name == NULL)) {
    cerr << "set_structured_predict_hook: searn task not initialized properly" << endl;
    throw exception();
  }
  if (strcmp(srn->task->task_name, "python_hook") != 0) {
    cerr << "set_structured_predict_hook: trying to set hook when searn task is not 'python_hook'!" << endl;
    throw exception();
  }
}  

uint32_t searn_get_num_actions(searn_ptr srn) {
  verify_searn_set_properly(srn);
  PythonTask::task_data* d = srn->get_task_data<PythonTask::task_data>();
  return d->num_actions;
}

void searn_run_fn(Searn::searn&srn) {
  try {
    PythonTask::task_data* d = srn.get_task_data<PythonTask::task_data>();
    py::object run = *(py::object*)d->run_object;
    run.attr("__call__")();
  } catch(...) {
    PyErr_Print();
    PyErr_Clear();
    throw exception();
  }
}

void set_structured_predict_hook(searn_ptr srn, py::object run_object) {
  verify_searn_set_properly(srn);
  PythonTask::task_data* d = srn->get_task_data<PythonTask::task_data>();
  d->run_f = &searn_run_fn;
  py::object* new_obj = new py::object(run_object);  // TODO: delete me!
  d->run_object = new_obj;
}

void my_set_test_only(example_ptr ec, bool val) { ec->test_only = val; }

bool po_exists(searn_ptr srn, string arg) {
  PythonTask::task_data* d = srn->get_task_data<PythonTask::task_data>();
  return d->var_map.count(arg) > 0;
}

string po_get_string(searn_ptr srn, string arg) {
  PythonTask::task_data* d = srn->get_task_data<PythonTask::task_data>();
  return d->var_map[arg].as<string>();
}

int32_t po_get_int(searn_ptr srn, string arg) {
  PythonTask::task_data* d = srn->get_task_data<PythonTask::task_data>();
  try { return d->var_map[arg].as<int>(); } catch (...) {}
  try { return d->var_map[arg].as<size_t>(); } catch (...) {}
  try { return d->var_map[arg].as<uint32_t>(); } catch (...) {}
  try { return d->var_map[arg].as<uint64_t>(); } catch (...) {}
  try { return d->var_map[arg].as<uint16_t>(); } catch (...) {}
  try { return d->var_map[arg].as<int32_t>(); } catch (...) {}
  try { return d->var_map[arg].as<int64_t>(); } catch (...) {}
  try { return d->var_map[arg].as<int16_t>(); } catch (...) {}
  // we know this'll fail but do it anyway to get the exception
  return d->var_map[arg].as<int>();
}

PyObject* po_get(searn_ptr srn, string arg) {
  try {
    return py::incref(py::object(po_get_string(srn, arg)).ptr());
  } catch (...) {}
  try {
    return py::incref(py::object(po_get_int(srn, arg)).ptr());
  } catch (...) {}
  // return None
  return py::incref(py::object().ptr());
}

//BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(searn_predict_overloads,    Searn::searn::predict,    2, 3);
//BOOST_PYTHON_MEMBER_FUNCTION_OVERLOADS(searn_predictLDF_overloads, Searn::searn::predictLDF, 3, 4);

BOOST_PYTHON_MODULE(pylibvw) {
  // This will enable user-defined docstrings and python signatures,
  // while disabling the C++ signatures
  py::docstring_options local_docstring_options(true, true, false);
  
  // define the vw class
  py::class_<vw, vw_ptr>("vw", "the basic VW object that holds with weight vector, parser, etc.", py::no_init)
      .def("__init__", py::make_constructor(my_initialize))
      //      .def("__del__", &my_finish, "deconstruct the VW object by calling finish")
      .def("finish", &my_finish, "stop VW by calling finish (and, eg, write weights to disk)")
      .def("learn", &my_learn, "given a pyvw example, learn (and predict) on that example")
      .def("learn_string", &my_learn_string, "given an example specified as a string (as in a VW data file), learn on that example")
      .def("hash_space", &VW::hash_space, "given a namespace (as a string), compute the hash of that namespace")
      .def("hash_feature", &VW::hash_feature, "given a feature string (arg2) and a hashed namespace (arg3), hash that feature")
      .def("finish_example", &my_finish_example, "tell VW that you're done with a given example")
      .def("setup_example", &my_setup_example, "given an example that you've created by hand, prepare it for learning (eg, compute quadratic feature)")

      .def("num_weights", &VW::num_weights, "how many weights are we learning?")
      .def("get_weight", &VW::get_weight, "get the weight for a particular index")
      .def("set_weight", &VW::set_weight, "set the weight for a particular index")
      .def("get_stride", &VW::get_stride, "return the internal stride")

      .def("get_sum_loss", &get_sum_loss, "return the total cumulative loss suffered so far")
      .def("get_weighted_examples", &get_weighted_examples, "return the total weight of examples so far")

      .def("get_searn_ptr", &get_searn_ptr, "return a pointer to the searn data structure")
      ;

  // define the example class
  py::class_<example, example_ptr>("example", py::no_init)
      .def("__init__", py::make_constructor(my_read_example), "Given a string as an argument parse that into a VW example (and run setup on it)")
      .def("__init__", py::make_constructor(my_empty_example), "Construct an empty (non setup) example")

      .def("set_test_only", &my_set_test_only, "Change the test-only bit on an example")

      .def("get_tag", &my_get_tag, "Returns the tag associated with this example")
      .def("get_topic_prediction", &VW::get_topic_prediction, "For LDA models, returns the topic prediction for the topic id given")
      .def("get_feature_number", &VW::get_feature_number, "Returns the total number of features for this example")

      // the following four are redundant with get_simplelabel_...
      //.def("get_label", &VW::get_label, "Returns the label (simple_label only!) for this example")
      //.def("get_importance", &VW::get_importance, "Returns the importance (simple_label only!) for this example")
      //.def("get_initial", &VW::get_initial, "TODO")
      //.def("get_prediction", &VW::get_prediction, "TODO")
      
      .def("get_example_counter", &get_example_counter, "Returns the counter of total number of examples seen up to and including this one")
      .def("get_ft_offset", &get_ft_offset, "Returns the feature offset for this example (used, eg, by multiclass classification to bulk offset all features)")
      .def("get_partial_prediction", &get_partial_prediction, "Returns the partial prediction associated with this example")
      .def("get_updated_prediction", &get_updated_prediction, "Returns the partial prediction as if we had updated it after learning")
      .def("get_loss", &get_loss, "Returns the loss associated with this example")
      .def("get_example_t", &get_example_t, "The total sum of importance weights up to and including this example")
      .def("get_total_sum_feat_sq", &get_total_sum_feat_sq, "The total sum of feature-value squared for this example")
      //      .def_readwrite("revert_weight", &example::revert_weight)
      //      .def_readwrite("test_only", &example::test_only)
      //      .def_readwrite("end_pass", &example::end_pass)
      //      .def_readwrite("sorted", &example::sorted)
      //      .def_readwrite("in_use", &example::in_use)

      .def("num_namespaces", &ex_num_namespaces, "The total number of namespaces associated with this example")
      .def("namespace", &ex_namespace, "Get the namespace id for namespace i (for i = 0.. num_namespaces); specifically returns the ord() of the corresponding character id")
      .def("sum_feat_sq", &ex_sum_feat_sq, "Get the sum of feature-values squared for a given namespace id (id=character-ord)")
      .def("num_features_in", &ex_num_features, "Get the number of features in a given namespace id (id=character-ord)")
      .def("feature", &ex_feature, "Get the feature id for the ith feature in a given namespace id (id=character-ord)")
      .def("feature_weight", &ex_feature_weight, "The the feature value (weight) per .feature(...)")

      .def("push_hashed_feature", &ex_push_feature, "Add a hashed feature to a given namespace (id=character-ord)")
      .def("pop_feature", &ex_pop_feature, "Remove the top feature from a given namespace; returns True iff the list was non-empty")
      .def("push_namespace", &ex_push_namespace, "Add a new namespace")
      .def("ensure_namespace_exists", &ex_ensure_namespace_exists, "Add a new namespace if it doesn't already exist")
      .def("pop_namespace", &ex_pop_namespace, "Remove the top namespace off; returns True iff the list was non-empty")

      .def("set_label_string", &ex_set_label_string, "(Re)assign the label of this example to this string")
      
      .def("get_simplelabel_label", &ex_get_simplelabel_label, "Assuming a simple_label label type, return the corresponding label (class/regression target/etc.)")
      .def("get_simplelabel_weight", &ex_get_simplelabel_weight, "Assuming a simple_label label type, return the importance weight")
      .def("get_simplelabel_initial", &ex_get_simplelabel_initial, "Assuming a simple_label label type, return the initial (baseline) prediction")
      .def("get_simplelabel_prediction", &ex_get_simplelabel_prediction, "Assuming a simple_label label type, return the final prediction")
      .def("get_multiclass_label", &ex_get_multiclass_label, "Assuming a multiclass label type, get the true label")
      .def("get_multiclass_weight", &ex_get_multiclass_weight, "Assuming a multiclass label type, get the importance weight")
      .def("get_multiclass_prediction", &ex_get_multiclass_prediction, "Assuming a multiclass label type, get the prediction")
      .def("get_costsensitive_prediction", &ex_get_costsensitive_prediction, "Assuming a cost_sensitive label type, get the prediction")
      .def("get_costsensitive_num_costs", &ex_get_costsensitive_num_costs, "Assuming a cost_sensitive label type, get the total number of label/cost pairs")
      .def("get_costsensitive_cost", &ex_get_costsensitive_cost, "Assuming a cost_sensitive label type, get the cost for a given pair (i=0.. get_costsensitive_num_costs)")
      .def("get_costsensitive_class", &ex_get_costsensitive_class, "Assuming a cost_sensitive label type, get the label for a given pair (i=0.. get_costsensitive_num_costs)")
      .def("get_costsensitive_partial_prediction", &ex_get_costsensitive_partial_prediction, "Assuming a cost_sensitive label type, get the partial prediction for a given pair (i=0.. get_costsensitive_num_costs)")
      .def("get_costsensitive_wap_value", &ex_get_costsensitive_wap_value, "Assuming a cost_sensitive label type, get the weighted-all-pairs recomputed cost for a given pair (i=0.. get_costsensitive_num_costs)")
      .def("get_cbandits_prediction", &ex_get_cbandits_prediction, "Assuming a contextual_bandits label type, get the prediction")
      .def("get_cbandits_num_costs", &ex_get_cbandits_num_costs, "Assuming a contextual_bandits label type, get the total number of label/cost pairs")
      .def("get_cbandits_cost", &ex_get_cbandits_cost, "Assuming a contextual_bandits label type, get the cost for a given pair (i=0.. get_cbandits_num_costs)")
      .def("get_cbandits_class", &ex_get_cbandits_class, "Assuming a contextual_bandits label type, get the label for a given pair (i=0.. get_cbandits_num_costs)")
      .def("get_cbandits_probability", &ex_get_cbandits_probability, "Assuming a contextual_bandits label type, get the bandits probability for a given pair (i=0.. get_cbandits_num_costs)")
      .def("get_cbandits_partial_prediction", &ex_get_cbandits_partial_prediction, "Assuming a contextual_bandits label type, get the partial prediction for a given pair (i=0.. get_cbandits_num_costs)")
      ;

  py::class_<Searn::searn, searn_ptr>("searn")
      .def("set_options", &Searn::searn::set_options, "Set global searn options (auto history, etc.)")
      .def("loss", &Searn::searn::loss, "Declare a (possibly incremental) loss")
      .def("predict_one_all", &searn_predict_one_all, "Predict with one true label and all labels valid (non-LDF)")
      .def("predict_many_all", &searn_predict_many_all, "Predict with several true labels and all labels valid (non-LDF)")
      .def("predict_one_some", &searn_predict_one_some, "Predict with one true label and only some labels valid (non-LDF)")
      .def("predict_many_some", &searn_predict_many_some, "Predict with several true labels and only some labels valid (non-LDF)")

      .def("should_output", &searn_should_output, "Check whether searn wants us to output (only happens if you have -p running)")
      .def("output", &searn_output, "Add a string to the coutput (should only do if should_output returns True)")
      .def("get_num_actions", &searn_get_num_actions, "Return the total number of actions searn was initialized with")
      .def("set_structured_predict_hook", &set_structured_predict_hook, "Set the hook (function pointer) that searn should use for structured prediction (you don't want to call this yourself!")

      .def("po_exists", &po_exists, "For program (cmd line) options, check to see if a given option was specified; eg srn.po_exists(\"search\") should be True")
      .def("po_get", &po_get, "For program (cmd line) options, if an option was specified, get its value; eg srn.po_get(\"search\") should return the # of actions (returns either int or string)")
      .def("po_get_str", &po_get_string, "Same as po_get, but specialized for string return values.")
      .def("po_get_int", &po_get_int, "Same as po_get, but specialized for integer return values.")
      
      .def_readonly("AUTO_HISTORY", Searn::AUTO_HISTORY, "Tell search to automatically add history to the feature set")
      .def_readonly("AUTO_HAMMING_LOSS", Searn::AUTO_HAMMING_LOSS, "Tell search to automatically compute hamming loss over predictions")
      .def_readonly("EXAMPLES_DONT_CHANGE", Searn::EXAMPLES_DONT_CHANGE, "Tell search that on a single structured 'run', you don't change the examples you pass to predict")
      .def_readonly("IS_LDF", Searn::IS_LDF, "Tell search that this is an LDF task")
      ;
}

