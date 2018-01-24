/* ----------------------------------------------------------------------- *//**
 *
 * @file linear_svm.hpp
 *
 *//* ----------------------------------------------------------------------- */

#ifndef MADLIB_MODULES_CONVEX_TASK_LINEAR_SVM_HPP_
#define MADLIB_MODULES_CONVEX_TASK_LINEAR_SVM_HPP_

namespace madlib {

namespace modules {

namespace convex {

// Use Eigen
using namespace madlib::dbal::eigen_integration;


template <class Model, class Tuple>
class LinearSVM {
public:
    typedef Model model_type;
    typedef Tuple tuple_type;

    typedef typename Tuple::independent_variables_type independent_variables_type;
    typedef typename Tuple::dependent_variable_type dependent_variable_type;

    // Model is assumed to be base Eigen type or Eigen map and the 'EigenType'
    // variable infers the actual type from the Model definition.
    // For eg. SVMModel is defined as a ColumnVectorTransparentHandleMap which
    // has a ColumnVector as its EigenType.
    typedef typename model_type::PlainEigenType coefficient_type;

    static double epsilon;
    static bool is_svc;

    static void gradient(
            const model_type                    &model,
            const independent_variables_type    &x,
            const dependent_variable_type       &y,
            model_type                          &gradient);

    static void gradientInPlace(
            model_type                          &model,
            const independent_variables_type    &x,
            const dependent_variable_type       &y,
            const double                        &stepsize);

    static double getLossAndUpdateModel(
            model_type                          &model,
            const independent_variables_type    &x,
            const dependent_variable_type       &y,
            const double                        &stepsize);

    static double loss(
            const model_type                    &model,
            const independent_variables_type    &x,
            const dependent_variable_type       &y);

    static dependent_variable_type predict(
            const model_type                    &model,
            const independent_variables_type    &x);
};

template <class Model, class Tuple>
double LinearSVM<Model, Tuple >::epsilon = 0.;

template <class Model, class Tuple>
bool LinearSVM<Model, Tuple >::is_svc = false;

template <class Model, class Tuple>
void
LinearSVM<Model, Tuple>::gradient(
        const model_type                    &model,
        const independent_variables_type    &x,
        const dependent_variable_type       &y,
        model_type                          &gradient) {
    double wx = dot(model, x);
    if (is_svc) {
        if (1 - wx * y > 0) {
            double c = -y;   // minus for "-loglik"
            gradient += c * x;
        }
    }
    else {
        double wx_y = wx - y;
        double c = wx_y > 0 ? 1. : -1.;
        if (c*wx_y - epsilon > 0.)
            gradient += c * x;
    }
}

template <class Model, class Tuple>
void
LinearSVM<Model, Tuple>::gradientInPlace(
        model_type                          &model,
        const independent_variables_type    &x,
        const dependent_variable_type       &y,
        const double                        &stepsize) {
    double wx = dot(model, x);
    if (is_svc) {
        if (1. - wx * y > 0.) {
            double c = -y;   // minus for "-loglik"
            model -= stepsize * c * x;
        }
    }
    else {
        double wx_y = wx - y;
        double c = wx_y > 0 ? 1. : -1.;
        if (c*wx_y - epsilon > 0.)
            model -= stepsize * c * x;
    }
}

/**
* @brief This function will update the model for a single batch and return the loss
* @param model Model to update
* @param x Batch of independent variables
* @param y Batch of dependent variables
* @param stepsize Learning rate for model update
* @return Total loss in the batch
*/
template <class Model, class Tuple>
double
LinearSVM<Model, Tuple>::getLossAndUpdateModel(
        model_type                          &model,
        const independent_variables_type    &x,
        const dependent_variable_type       &y,
        const double                        &stepsize){

    // This function is called by the minibatch transition function to update
    // the model for each batch. x and y in the function signature are defined
    // as generic variables to ensure a consistent interface across all modules.

    // ASSUMPTION: 'gradient' will always be of the same type as the
    // coefficients. In SVM, the model is just the coefficients, but can be
    // more complex with other modules like MLP.
    coefficient_type gradient = model;
    gradient.setZero();
    coefficient_type w_transpose_x = x * model;
    double loss = 0.0;
    int batch_size = x.rows();
    double dist_from_hyperplane = 0.0;
    double c = 0.0;
    int n_points_with_positive_dist = 0;
    for (int i = 0; i < batch_size; i++) {
        if (is_svc) {
            c = -y(i);   // minus for "-loglik"
            dist_from_hyperplane = 1.0 - w_transpose_x(i) * y(i);
        } else {
            double wx_y = w_transpose_x(i) - y(i);
            c = wx_y > 0 ? 1.0 : -1.0;
            dist_from_hyperplane = c * wx_y - epsilon;
        }
        if (dist_from_hyperplane > 0.) {
            gradient += c * x.row(i);
            loss += dist_from_hyperplane;
            n_points_with_positive_dist++;
        }
    }
    gradient.array() /= n_points_with_positive_dist;
    model -= stepsize * gradient;
    return loss;
}

template <class Model, class Tuple>
double
LinearSVM<Model, Tuple>::loss(
        const model_type                    &model,
        const independent_variables_type    &x,
        const dependent_variable_type       &y) {
    double wx = dot(model, x);
    double distance = 0.;
    if (is_svc) {
        distance = 1. - wx * y;
    }
    else {
        distance = fabs(wx - y) - epsilon;
    }
    return distance > 0. ? distance : 0.;
}

template <class Model, class Tuple>
typename Tuple::dependent_variable_type
LinearSVM<Model, Tuple>::predict(
        const model_type                    &model,
        const independent_variables_type    &x) {
    return dot(model, x);
}

} // namespace convex

} // namespace modules

} // namespace madlib

#endif
